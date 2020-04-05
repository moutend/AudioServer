package core

import (
	"log"

	"github.com/go-ole/go-ole"
	"github.com/moutend/AudioServer/pkg/com"
	"github.com/moutend/AudioServer/pkg/types"
)

var (
	isRunning bool
	server    *com.IAudioServer
)

func Setup() error {
	if isRunning {
		return nil
	}
	if err := ole.CoInitializeEx(0, ole.COINIT_MULTITHREADED); err != nil {
		return err
	}
	if err := com.CoCreateInstance(com.CLSID_AudioServer, 0, com.CLSCTX_ALL, com.IID_IAudioServer, &server); err != nil {
		return err
	}
	if err := server.Start(); err != nil {
		return err
	}

	isRunning = true

	log.Println("core: Setup() is done")

	return nil
}

func Teardown() error {
	server.Release()
	server.Stop()

	ole.CoUninitialize()

	isRunning = false

	log.Println("core: Teardown() is done")

	return nil
}

func Push(isForcePush bool, commands []types.Command) error {
	if len(commands) == 0 {
		return nil
	}
	if err := server.Push(isForcePush, commands); err != nil {
		return err
	}

	log.Println("core: Push() is done")

	return nil
}

func SetDefaultVoiceProperty(property *types.VoiceProperty) error {
	index, err := server.GetDefaultVoice()

	if err != nil {
		return err
	}
	if err := server.SetVoiceProperty(index, property); err != nil {
		return err
	}

	return nil
}

func GetDefaultVoiceProperty() (*types.VoiceProperty, error) {
	index, err := server.GetDefaultVoice()

	if err != nil {
		return nil, err
	}

	property, err := server.GetVoiceProperty(index)

	if err != nil {
		return nil, err
	}

	return property, nil
}

func FadeIn() error {
	if err := server.FadeIn(); err != nil {
		return err
	}

	return nil
}

func FadeOut() error {
	if err := server.FadeOut(); err != nil {
		return err
	}

	return nil
}
