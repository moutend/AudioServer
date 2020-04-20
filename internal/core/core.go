package core

import (
	"log"
	"os/user"
	"path/filepath"

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
	if err := ole.CoInitializeEx(0, ole.COINIT_APARTMENTTHREADED); err != nil {
		return err
	}
	if err := com.CoCreateInstance(com.CLSID_AudioServer, 0, com.CLSCTX_ALL, com.IID_IAudioServer, &server); err != nil {
		return err
	}

	myself, err := user.Current()

	if err != nil {
		return err
	}
	soundEffectsPath := filepath.Join(myself.HomeDir, "AppData", "Roaming", "ScreenReaderX", "SoundEffect")
	loggerURL := ""

	if err := server.Start(soundEffectsPath, loggerURL, 0); err != nil {
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

func SetVoiceProperty(index int32, property *types.VoiceProperty) error {
	if err := server.SetVoiceProperty(index, property); err != nil {
		return err
	}

	return nil
}

func GetVoiceCount() (int32, error) {
	count, err := server.GetVoiceCount()

	if err != nil {
		return -1, err
	}

	return count, nil
}

func GetDefaultVoice() (int32, error) {
	index, err := server.GetDefaultVoice()

	if err != nil {
		return -1, err
	}

	return index, nil
}

func GetVoiceProperty(index int32) (*types.VoiceProperty, error) {
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
