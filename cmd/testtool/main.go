package main

import (
	"fmt"
	"log"
	"os"
	"time"
	"unsafe"

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

	err := audioServer.Start()
	fmt.Println("Called IAudioServer::Start", err)

	err = audioServer.SetNotifyIdleStateHandler(func(v int64) int64 {
		fmt.Println("@@@idle", v)

		return 0
	})
	fmt.Println("Called IAudioServer::SetNotifyIdleStateHandler", err)

	err = audioServer.Push([]types.Command{
		{
			Type:         3,
			TextToSpeech: "こんにちは、私の名前は京子です。日本語の音声をお届けします。",
		},
	})
	fmt.Println("Called IAudioServer::()Push", err)

	time.Sleep(9 * time.Second)

	index, err := audioServer.GetDefaultVoice()

	if err != nil {
		return err
	}

	property, err := audioServer.GetVoiceProperty(index)

	if err != nil {
		return err
	}

	defer ole.CoTaskMemFree(property.RawVoiceProperty.Language)
	defer ole.CoTaskMemFree(property.RawVoiceProperty.DisplayName)
	defer ole.CoTaskMemFree(uintptr(unsafe.Pointer(&(property.RawVoiceProperty))))

	fmt.Printf("@@@property %+v\n", property)

	property.SpeakingRate += 1.0

	err = audioServer.SetVoiceProperty(index, property)

	if err != nil {
		return err
	}

	err = audioServer.Push([]types.Command{
		{
			Type:         3,
			TextToSpeech: "こんにちは、私の名前は京子です。日本語の音声をお届けします。",
		},
	})

	fmt.Println("Called IAudioServer::()Push", err)

	time.Sleep(9 * time.Second)

	if err != nil {
		return err
	}

	fmt.Println("@@@done")

	return nil
}
