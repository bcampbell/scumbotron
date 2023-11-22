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
	"regexp"
	"strings"
)

var opts struct {
	palette   string
	width     int
	height    int
	numFrames int
	fmt       string
	csource   bool
	varName   string
}

func main() {
	flag.Usage = func() {
		txt := `Usage: %s [flags] infile [outfile]

Convert a paletted png image to sprites.

Flags:
`
		fmt.Fprintf(os.Stderr, txt, os.Args[0])
		flag.PrintDefaults()
	}

	flag.StringVar(&opts.palette, "palette", "", "Output palette only (RGBA, nds, cx16)")
	flag.IntVar(&opts.width, "w", 8, "Sprite width")
	flag.IntVar(&opts.height, "h", 8, "Sprite height")
	flag.IntVar(&opts.numFrames, "num", 1, "Number of sprites to export")
	flag.StringVar(&opts.fmt, "fmt", "8bpp", "Output format (8bpp, 4bpp, 1bpp, nds4bpp, mono4x2)")
	flag.BoolVar(&opts.csource, "c", false, "Output C src instead of raw binary")
	flag.StringVar(&opts.varName, "var", "", "Set variable name used for C source output (-c) (defaults to munged filename)")
	flag.Parse()

	if len(flag.Args()) < 1 {
		flag.Usage()
		os.Exit(2)
	}

	inFile := flag.Arg(0)
	img, err := loadImg(inFile)
	if err != nil {
		fmt.Fprintf(os.Stderr, "ERR: %s\n", err)
		os.Exit(1)
	}

	// output file specified?
	var outFile string
	if len(flag.Args()) >= 2 {
		outFile = flag.Arg(1)
	}
	dumper, err := initDumper(inFile, outFile)
	if err != nil {
		fmt.Fprintf(os.Stderr, "ERR: %s\n", err)
		os.Exit(1)
	}
	defer dumper.Close()

	if opts.palette != "" {
		// dump palette only.
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

func initDumper(inFile string, outFile string) (Dumper, error) {
	var f *os.File

	if outFile == "-" || outFile == "" {
		f = os.Stdout
	} else {
		var err error
		f, err = os.Create(outFile)
		if err != nil {
			return nil, err
		}
	}

	if opts.csource {
		v := opts.varName
		if v == "" {
			v = inFile
		}
		// Sanitise name for C.
		re := regexp.MustCompile("[^a-zA-Z0-9_]")
		v = re.ReplaceAllLiteralString(v, "_")
		if len(v) > 0 && v[0] >= '0' && v[0] <= '9' {
			v = "__" + v
		}
		d := &SrcDumper{VarName: v, NumPerRow: 8, Out: f, Indent: "\t"}
		return d, nil
	} else {
		return &BinDumper{f}, nil
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

	switch opts.palette {
	case "nds":
		return dumpPaletteNDS(m, dumper)
	case "cx16":
		return dumpPaletteCX16(m, dumper)
	case "rgba":
		return dumpPaletteRGBA(m, dumper)
	default:
		return errors.New("Unknown -palette type")
	}
}

func dumpPaletteRGBA(m *image.Paletted, dumper Dumper) error {
	var pal color.Palette
	pal = m.ColorModel().(color.Palette)
	dumper.Commentf("Palette (%d colours) R,G,B,A\n", len(pal))
	for i := 0; i < len(pal); i++ {
		c := pal[i].(color.RGBA)
		_, err := dumper.Write([]byte{c.R, c.G, c.B, c.A})
		if err != nil {
			return err
		}
	}
	return nil
}

func dumpPaletteCX16(m *image.Paletted, dumper Dumper) error {
	var pal color.Palette
	pal = m.ColorModel().(color.Palette)
	dumper.Commentf("Palette (%d colours) xxxxrrrrggggbbbb, little endian\n", len(pal))
	for i := 0; i < len(pal); i++ {
		c := pal[i].(color.RGBA)
		// output format is
		// xxxxrrrrggggbbbb, little endian

		lo := uint8((c.G & 0xf0) | (c.B >> 4))
		hi := uint8(c.R >> 4)
		_, err := dumper.Write([]byte{lo, hi})
		if err != nil {
			return err
		}
	}
	return nil
}

func dumpPaletteNDS(m *image.Paletted, dumper Dumper) error {
	var pal color.Palette
	pal = m.ColorModel().(color.Palette)
	dumper.Commentf("Palette (%d colours) xbbbbgggggrrrrr little-endian\n", len(pal))
	for i := 0; i < len(pal); i++ {
		c := pal[i].(color.RGBA)
		// output format is
		// xbbbbbgggggrrrrr (but in little-endian order)
		var out uint16
		out = uint16(c.R>>3) | (uint16(c.G>>3) << 5) | (uint16(c.B>>3) << 10)
		_, err := dumper.Write([]byte{uint8(out & 0xff), uint8(out >> 8)})
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
		dat, err := cook(frame)
		if err != nil {
			return err
		}
		err = writer.Commentf("img %d (%dx%d)\n", i, w, h)
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

func cook(img *image.Paletted) ([]uint8, error) {
	switch opts.fmt {
	case "8bpp":
		return cook8bpp(img)
	case "4bpp":
		return cook4bpp(img)
	case "nds4bpp":
		return cook4bppNDS(img)
	case "1bpp":
		return cook1bpp(img)
	case "mono4x2":
		return cookMono4x2(img)
	}
	return []uint8{}, errors.New("unsupported -fmt")
}

func cook8bpp(img *image.Paletted) ([]uint8, error) {
	out := []uint8{}
	r := img.Bounds()
	for y := 0; y < img.Bounds().Dy(); y++ {
		idx := img.PixOffset(r.Min.X, r.Min.Y+y)
		out = append(out, img.Pix[idx:idx+r.Dx()]...)
	}
	return out, nil
}

func cook4bpp(img *image.Paletted) ([]uint8, error) {
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
	return out, nil
}

// NDS 4bbp
// outputs 8x8 blocks (so 32x16 sprite would be 8 blocks)
// 2 pixels/byte, but nds needs reversed order!
func cook4bppNDS(img *image.Paletted) ([]uint8, error) {
	out := []uint8{}
	r := img.Bounds()
	// TODO: return error if not divisible into 8x8 blocks.
	for blky := 0; blky < r.Dy()/8; blky++ {
		for blkx := 0; blkx < r.Dx()/8; blkx++ {
			for y := 0; y < 8; y++ {
				idx := img.PixOffset(r.Min.X+(blkx*8), r.Min.Y+(blky*8)+y)
				for x := 0; x < 8; x += 2 {
					// note reversed order!
					b := (img.Pix[idx] & 0x0f) | (img.Pix[idx+1]&0x0f)<<4
					idx += 2
					out = append(out, b)
				}
			}
		}
	}
	return out, nil
}

func cook1bpp(img *image.Paletted) ([]uint8, error) {
	out := []uint8{}
	r := img.Bounds()
	for y := 0; y < r.Dy(); y++ {
		idx := img.PixOffset(r.Min.X, r.Min.Y+y)
		for x := 0; x < r.Dx(); x += 8 {
			var b byte
			for i := 7; i >= 0; i-- {
				if img.Pix[idx] > 0 {
					b |= 1 << i
				}
				idx++
			}
			out = append(out, b)
		}
	}
	return out, nil
}

// Each output byte encodes 2 2x2 blocks of pixels.
// used for old school petscii-style 4-pixels-in-a-character.
// Bit ordering of pixels:
//
// 5410
// 7632
//
// So, the low 4 bits can be used as the right character,
// and the hi 4 bits can be used as the left.

func cookMono4x2(img *image.Paletted) ([]uint8, error) {
	out := []uint8{}
	r := img.Bounds()
	for y := 0; y < r.Dy(); y += 2 {
		for x := 0; x < r.Dx(); x += 4 {
			row0 := img.PixOffset(r.Min.X+x, r.Min.Y+y)
			row1 := img.PixOffset(r.Min.X+x, r.Min.Y+y+1)
			var b byte
			// left char in low 4 bits:
			if img.Pix[row0+3] > 0 {
				b |= 1 << 0
			}
			if img.Pix[row0+2] > 0 {
				b |= 1 << 1
			}
			if img.Pix[row1+3] > 0 {
				b |= 1 << 2
			}
			if img.Pix[row1+2] > 0 {
				b |= 1 << 3
			}
			// right char in low 4 bits:
			if img.Pix[row0+1] > 0 {
				b |= 1 << 4
			}
			if img.Pix[row0+0] > 0 {
				b |= 1 << 5
			}
			if img.Pix[row1+1] > 0 {
				b |= 1 << 6
			}
			if img.Pix[row1+0] > 0 {
				b |= 1 << 7
			}
			out = append(out, b)
		}
	}
	return out, nil
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
//
// unsigned char export_foo[] = {
//     ...hex bytes...
// };
// unsigned int export_foo_len = 16384;
//

type SrcDumper struct {
	VarName       string // Name for output array & length
	NumPerRow     int
	Out           io.WriteCloser
	Indent        string
	parts         []string
	bytesWritten  int
	headerWritten bool // array definition written out?
}

func (d *SrcDumper) Write(p []byte) (n int, err error) {
	for _, b := range p {
		d.parts = append(d.parts, fmt.Sprintf("0x%02x,", b))
		if len(d.parts) >= d.NumPerRow {
			err = d.flush()
			if err != nil {
				return 0, err
			}
		}
	}
	d.bytesWritten += len(p)
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
	if err == nil {
		// The footer.
		_, err = fmt.Fprintf(d.Out, "};\nconst unsigned int %s_len = %d;\n", d.VarName, d.bytesWritten)
	}
	err2 := d.Out.Close()
	if err != nil {
		return err
	}
	return err2
}

func (d *SrcDumper) flush() error {
	var err error
	if !d.headerWritten {
		d.headerWritten = true
		_, err = fmt.Fprintf(d.Out, "const unsigned char %s[] = {\n", d.VarName)
		if err != nil {
			return err
		}
	}

	if len(d.parts) == 0 {
		return nil
	}
	_, err = fmt.Fprintf(d.Out, "%s%s\n", d.Indent,
		strings.Join(d.parts, " "))
	d.parts = []string{}
	return err
}
