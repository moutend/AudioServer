package main

import (
	"fmt"
	"log"
	"os"
	"time"

	"github.com/moutend/AudioServer/pkg/com"
	"github.com/moutend/AudioServer/pkg/types"

	"github.com/go-ole/go-ole"
)

func main() {
	log.SetPrefix("error: ")
	log.SetFlags(0)

	if err := run(os.Args); err != nil {
		log.Fatal(err)
	}
}

func run(args []string) error {
	if err := ole.CoInitializeEx(0, ole.COINIT_MULTITHREADED); err != nil {
		return err
	}
	defer ole.CoUninitialize()

	var audioServer *com.IAudioServer

	if err := com.CoCreateInstance(com.CLSID_AudioServer, 0, com.CLSCTX_ALL, com.IID_IAudioServer, &audioServer); err != nil {
		return err
	}
	defer audioServer.Stop()
	defer audioServer.Release()

	audioServer.Start()
	fmt.Println("Start audio server")
	time.Sleep(3 * time.Second)
	audioServer.Push([]types.Command{
		{
			Type:         3,
			TextToSpeech: "こんにちは",
		},
	})
	time.Sleep(10 * time.Second)
	return nil
}
