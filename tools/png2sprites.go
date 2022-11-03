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
	paletteOnly bool
	width       int
	height      int
	numFrames   int
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

	flag.BoolVar(&opts.paletteOnly, "p", false, "Just output palette")
	flag.IntVar(&opts.width, "w", 8, "width")
	flag.IntVar(&opts.height, "h", 8, "width")
	flag.IntVar(&opts.numFrames, "n", 1, "number of frames")

	flag.Parse()

	if len(flag.Args()) < 1 {
		flag.Usage()
		os.Exit(2)
	}

	img, err := loadImg(flag.Arg(0))
	if err != nil {
		fmt.Fprintf(os.Stdout, "ERR: %s\n", err)
		os.Exit(1)
	}

	if opts.paletteOnly {
		err := dumpPalette(img, os.Stdout)
		if err != nil {
			fmt.Fprintf(os.Stdout, "ERR: %s\n", err)
			os.Exit(1)
		}
		return
	}

	err = dumpImages(img, os.Stdout)
	if err != nil {
		fmt.Fprintf(os.Stdout, "ERR: %s\n", err)
		os.Exit(1)
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

func dumpPalette(m *image.Paletted, out io.Writer) error {
	var pal color.Palette
	pal = m.ColorModel().(color.Palette)

	fmt.Fprintf(out, "// Palette (%d colours)\n", len(pal))
	for i := 0; i < len(pal); i++ {
		c := pal[i].(color.RGBA)
		// output format is
		// ggggbbbb, xxxxrrrr

		lo := uint8((c.G & 0xf0) | (c.B >> 4))
		hi := uint8(c.R >> 4)
		fmt.Fprintf(out, "\t0x%02x, 0x%02x,\n", lo, hi)
		//	fmt.Printf("%d,%d,%d => 0x%02x\n", c.R, c.G, c.B, buf[i])
	}
	return nil
}

func dumpImages(img *image.Paletted, out io.Writer) error {

	w := opts.width
	h := opts.height

	writer := SrcDumper{NumPerRow: 8, Out: out, Indent: "\t"}
	defer writer.Close()

	x := 0
	y := 0
	for i := 0; i < opts.numFrames; i++ {
		rect := image.Rect(x, y, x+w, y+h)
		if !rect.In(img.Bounds()) {
			return fmt.Errorf("frame %d is outside image bounds", i)
		}
		frame, _ := img.SubImage(rect).(*image.Paletted)
		dat := cook(frame)
		err := writer.WriteRaw(fmt.Sprintf("\t// img %d (%dx%d)\n", i, w, h))
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
	out := []uint8{}
	r := img.Bounds()
	for y := 0; y < img.Bounds().Dy(); y++ {
		idx := img.PixOffset(r.Min.X, r.Min.Y+y)
		out = append(out, img.Pix[idx:idx+r.Dx()]...)
	}
	return out
}

// helper to write bytes out as C source code
// implements io.WriteCloser
type SrcDumper struct {
	NumPerRow int
	Out       io.Writer
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

func (d *SrcDumper) WriteRaw(s string) error {
	err := d.flush()
	if err != nil {
		return err
	}
	_, err = io.WriteString(d.Out, s)
	return err
}

func (d *SrcDumper) Close() error {
	return d.flush()
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
