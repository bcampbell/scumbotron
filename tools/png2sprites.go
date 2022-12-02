package main

import (
	"errors"
	"flag"
	"fmt"
	"image"
	"image/color"
	"image/png"
	"io"
	"os"
	"strings"
)

var opts struct {
	paletteOnly  bool
	width        int
	height       int
	numFrames    int
	bitsPerPixel int
	fmt          string
	outFile      string
}

func main() {
	flag.Usage = func() {
		txt := `Usage: %s [flags] infile

Convert a paletted png image to sprites.

Flags:
`
		fmt.Fprintf(os.Stderr, txt, os.Args[0])
		flag.PrintDefaults()
	}

	flag.BoolVar(&opts.paletteOnly, "palette", false, "Output palette only")
	flag.IntVar(&opts.width, "w", 8, "sprite width")
	flag.IntVar(&opts.height, "h", 8, "sprite height")
	flag.IntVar(&opts.numFrames, "num", 1, "number of sprites to export")
	flag.IntVar(&opts.bitsPerPixel, "bpp", 8, "Bits per pixel (8,4,2,1)")
	flag.StringVar(&opts.fmt, "fmt", "c", "Output format ('c', 'bin')")
	flag.StringVar(&opts.outFile, "out", "-", "Output file ('-' for stdout)")
	flag.Parse()

	if len(flag.Args()) < 1 {
		flag.Usage()
		os.Exit(2)
	}

	img, err := loadImg(flag.Arg(0))
	if err != nil {
		fmt.Fprintf(os.Stderr, "ERR: %s\n", err)
		os.Exit(1)
	}

	dumper, err := initDumper()
	if err != nil {
		fmt.Fprintf(os.Stderr, "ERR: %s\n", err)
		os.Exit(1)
	}
	defer dumper.Close()

	if opts.paletteOnly {
		err := dumpPalette(img, dumper)
		if err != nil {
			fmt.Fprintf(os.Stderr, "ERR: %s\n", err)
			os.Exit(1)
		}
		return
	}

	err = dumpImages(img, dumper)
	if err != nil {
		fmt.Fprintf(os.Stderr, "ERR: %s\n", err)
		os.Exit(1)
	}
}

func initDumper() (Dumper, error) {
	var f *os.File

	if opts.outFile == "-" {
		f = os.Stdout
	} else {
		var err error
		f, err = os.Create(opts.outFile)
		if err != nil {
			return nil, err
		}
	}

	switch opts.fmt {
	case "c":
		return &SrcDumper{NumPerRow: 8, Out: f, Indent: "\t"}, nil
	case "bin":
		return &BinDumper{f}, nil
	default:
		return nil, fmt.Errorf("Unknown format '%s'", opts.fmt)
	}
}

func loadImg(infilename string) (*image.Paletted, error) {
	// Open the infile.
	infile, err := os.Open(infilename)
	if err != nil {
		return nil, err
	}
	defer infile.Close()

	// Decode the image.
	m, err := png.Decode(infile)
	if err != nil {
		return nil, err
	}
	p, ok := m.(*image.Paletted)
	if !ok {
		return nil, errors.New("Image is not paletted")
	}
	return p, nil
}

func dumpPalette(m *image.Paletted, dumper Dumper) error {
	var pal color.Palette
	pal = m.ColorModel().(color.Palette)

	dumper.Commentf("Palette (%d colours)\n", len(pal))
	for i := 0; i < len(pal); i++ {
		c := pal[i].(color.RGBA)
		// output format is
		// ggggbbbb, xxxxrrrr

		lo := uint8((c.G & 0xf0) | (c.B >> 4))
		hi := uint8(c.R >> 4)
		_, err := dumper.Write([]byte{lo, hi})
		if err != nil {
			return err
		}
	}
	return nil
}

func dumpImages(img *image.Paletted, writer Dumper) error {

	w := opts.width
	h := opts.height

	x := 0
	y := 0
	for i := 0; i < opts.numFrames; i++ {
		rect := image.Rect(x, y, x+w, y+h)
		if !rect.In(img.Bounds()) {
			return fmt.Errorf("frame %d is outside image bounds", i)
		}
		frame, _ := img.SubImage(rect).(*image.Paletted)
		dat := cook(frame)
		err := writer.Commentf("img %d (%dx%d)\n", i, w, h)
		if err != nil {
			return err
		}
		_, err = writer.Write(dat)
		if err != nil {
			return err
		}

		// next
		x += w
		if x+w > img.Bounds().Dx() {
			x = 0
			y += h
		}
	}
	return nil
}

func cook(img *image.Paletted) []uint8 {
	switch opts.bitsPerPixel {
	case 8:
		return cook8bpp(img)
	case 4:
		return cook4bpp(img)
	}
	return []uint8{}
}

func cook8bpp(img *image.Paletted) []uint8 {
	out := []uint8{}
	r := img.Bounds()
	for y := 0; y < img.Bounds().Dy(); y++ {
		idx := img.PixOffset(r.Min.X, r.Min.Y+y)
		out = append(out, img.Pix[idx:idx+r.Dx()]...)
	}
	return out
}

func cook4bpp(img *image.Paletted) []uint8 {
	out := []uint8{}
	r := img.Bounds()
	for y := 0; y < r.Dy(); y++ {
		idx := img.PixOffset(r.Min.X, r.Min.Y+y)
		for x := 0; x < r.Dx(); x += 2 {
			b := (img.Pix[idx]&0x0f)<<4 | (img.Pix[idx+1] & 0x0f)
			idx += 2
			out = append(out, b)
		}
	}
	return out
}

type Dumper interface {
	io.WriteCloser
	Commentf(format string, args ...interface{}) error
}

// BinDumper adds a do-nothing Commentf() to a writer.
type BinDumper struct {
	io.WriteCloser
}

func (b *BinDumper) Commentf(format string, args ...interface{}) error { return nil }

// SrcDumper writes bytes out as C source code.
// implements io.WriteCloser and Commenter
type SrcDumper struct {
	NumPerRow int
	Out       io.WriteCloser
	Indent    string
	parts     []string
}

func (d *SrcDumper) Write(p []byte) (n int, err error) {
	for _, b := range p {
		d.parts = append(d.parts, fmt.Sprintf("0x%02x,", b))
		if len(d.parts) >= d.NumPerRow {
			err := d.flush()
			if err != nil {
				return 0, err
			}
		}
	}
	return len(p), nil
}

func (d *SrcDumper) Commentf(format string, args ...interface{}) error {
	err := d.flush()
	if err != nil {
		return err
	}
	s := d.Indent + "// " + fmt.Sprintf(format, args...)
	_, err = io.WriteString(d.Out, s)
	return err
}

func (d *SrcDumper) Close() error {
	err := d.flush()
	err2 := d.Out.Close()
	if err != nil {
		return err
	}
	return err2
}

func (d *SrcDumper) flush() error {
	if len(d.parts) == 0 {
		return nil
	}
	_, err := fmt.Fprintf(d.Out, "%s%s\n", d.Indent,
		strings.Join(d.parts, " "))
	d.parts = []string{}
	return err
}
