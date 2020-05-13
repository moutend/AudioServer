package main

import (
	"bytes"
	"fmt"
	"log"
	"mime"
	"net/http"
	"os/user"
	"path/filepath"
	"time"

	"github.com/nsf/termbox-go"
)

type command struct {
	Type  int         `json:"type"`
	Value interface{} `json:"value"`
}

type commandRequest struct {
	Commands []command `json:"commands"`
}

func main() {
	myself, err := user.Current()

	if err != nil {
		panic(err)
	}
	fileName := "keyboard.txt"
	outputPath := filepath.Join(myself.HomeDir, "AppData", "Roaming", "ScreenReaderX", "Logs", "SystemLog", fileName)
	output := NewBackgroundWriter(outputPath)

	defer output.Close()

	log.SetFlags(log.Ldate | log.Ltime | log.LUTC | log.Llongfile)
	log.SetOutput(output)

	contentType := mime.TypeByExtension(".json")

	err = termbox.Init()
	if err != nil {
		panic(err)
	}
	defer termbox.Close()

	for {
		switch ev := termbox.PollEvent(); ev.Type {
		case termbox.EventKey:
			switch ev.Key {
			case termbox.KeyEsc:
				fmt.Println("end")
				return
			case termbox.KeyArrowUp:
				go http.Post("http://localhost:7902/v1/audio/restart", `application/json`, nil)
			case termbox.KeyArrowDown:
				go http.Post("http://localhost:7902/v1/audio/pause", `application/json`, nil)
			case termbox.KeyArrowLeft:
			case termbox.KeyArrowRight:
			case termbox.KeySpace:
			default:
				// v := 1

				if ev.Ch == []rune("j")[0] {
					// v = 55
					go http.Post("http://localhost:7902/v1/voice/pitch?diff=-0.05", contentType, nil)
				}
				if ev.Ch == []rune("k")[0] {
					// v = 56
					go http.Post("http://localhost:7902/v1/voice/pitch?diff=0.05", contentType, nil)
				}
				if ev.Ch == []rune("h")[0] {
					// v = 66
					go http.Post("http://localhost:7902/v1/voice/rate?diff=-0.05", contentType, nil)
				}
				if ev.Ch == []rune("l")[0] {
					// v = 42
					go http.Post("http://localhost:7902/v1/voice/rate?diff=0.05", contentType, nil)
				}
				now := time.Now().UTC()
				if ev.Ch == []rune("1")[0] {
					log.Println("push (false)", now.Unix(), now.Nanosecond())
					go http.Post("http://localhost:7902/v1/audio", contentType, bytes.NewBufferString(`{"isForcePush":false,"commands":[{"type":1,"sfxIndex":10}]}`))
				} else {
					log.Println("push (true)", now.Unix(), now.Nanosecond())
					go http.Post("http://localhost:7902/v1/audio", contentType, bytes.NewBufferString(`{"isForcePush":true,"commands":[{"type":1,"sfxIndex":10}]}`))
				}
			}
		}
	}
}
