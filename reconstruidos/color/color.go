package color

import "fmt"

type Color struct{ codes []string }

func (c *Color) Add(x ...string) *Color {
	c.codes = append(c.codes, x...)
	return c
}

func (c *Color) Wrap(s string) string {
	pre := ""
	for _, x := range c.codes {
		pre += x
	}
	return pre + s + "\x1b[0m"
}

var (
	FgRed    = "\x1b[31m"
	FgGreen  = "\x1b[32m"
	FgYellow = "\x1b[33m"
	FgWhite  = "\x1b[97m"
	Bold     = "\x1b[1m"
	BackgroundRed = "\x1b[41m"
)

func New(codes ...string) *Color {
	cc := &Color{}
	for _, c := range codes {
		cc.codes = append(cc.codes, c)
	}
	return cc
}

var _ = fmt.Sprintf
