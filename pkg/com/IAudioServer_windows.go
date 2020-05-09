// +build windows

package com

import (
	"log"
	"syscall"
	"unsafe"

	"github.com/moutend/AudioServer/pkg/types"

	"github.com/go-ole/go-ole"
)

func asStart(v *IAudioServer, soundEffectsPath, loggerURL string, logLevel int) error {
	u16SoundEffectsPath, err := syscall.UTF16FromString(soundEffectsPath)

	if err != nil {
		return err
	}

	u16LoggerURL, err := syscall.UTF16FromString(loggerURL)

	if err != nil {
		return err
	}

	hr, _, _ := syscall.Syscall6(
		v.VTable().Start,
		4,
		uintptr(unsafe.Pointer(v)),
		uintptr(unsafe.Pointer(&u16SoundEffectsPath[0])),
		uintptr(unsafe.Pointer(&u16LoggerURL[0])),
		uintptr(logLevel),
		0,
		0)

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}

func asStop(v *IAudioServer) error {
	hr, _, _ := syscall.Syscall(
		v.VTable().Stop,
		1,
		uintptr(unsafe.Pointer(v)),
		0,
		0)

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}

func asFadeIn(v *IAudioServer) error {
	hr, _, _ := syscall.Syscall(
		v.VTable().FadeIn,
		1,
		uintptr(unsafe.Pointer(v)),
		0,
		0)

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}

func asFadeOut(v *IAudioServer) error {
	hr, _, _ := syscall.Syscall(
		v.VTable().FadeOut,
		1,
		uintptr(unsafe.Pointer(v)),
		0,
		0)

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}

func asPush(v *IAudioServer, isForcePush bool, commands []types.Command) error {
	cs := make([]types.RawCommand, len(commands), len(commands))
	ps := make([]uintptr, len(commands), len(commands))

	for i, c := range commands {
		cs[i] = types.RawCommand{
			Type:          c.Type,
			SFXIndex:      c.SFXIndex,
			SleepDuration: c.SleepDuration,
			Pan:           c.Pan,
		}
		if c.Type == 3 {
			u16ptr, err := syscall.UTF16PtrFromString(c.TextToSpeech)

			if err != nil {
				return err
			}

			cs[i].TextToSpeech = uintptr(unsafe.Pointer(u16ptr))
		}
		ps[i] = uintptr(unsafe.Pointer(&cs[i]))
	}

	isForcePushFlag := uintptr(0)

	if isForcePush {
		isForcePushFlag = 1
	}
	hr, _, _ := syscall.Syscall6(
		v.VTable().Push,
		4,
		uintptr(unsafe.Pointer(v)),
		uintptr(unsafe.Pointer(&ps[0])),
		uintptr(len(commands)),
		uintptr(isForcePushFlag),
		0,
		0)

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}

func asGetVoiceCount(v *IAudioServer) (int32, error) {
	var count int32

	hr, _, _ := syscall.Syscall(
		v.VTable().GetVoiceCount,
		1,
		uintptr(unsafe.Pointer(v)),
		uintptr(unsafe.Pointer(&count)),
		0)

	if hr != 0 {
		return -1, ole.NewError(hr)
	}

	return count, nil
}

func asGetDefaultVoice(v *IAudioServer) (int32, error) {
	var index int32

	hr, _, _ := syscall.Syscall(
		v.VTable().GetDefaultVoice,
		1,
		uintptr(unsafe.Pointer(v)),
		uintptr(unsafe.Pointer(&index)),
		0)

	if hr != 0 {
		return -1, ole.NewError(hr)
	}

	return index, nil
}

func asGetVoiceProperty(v *IAudioServer, index int32) (*types.VoiceProperty, error) {
	var property *types.RawVoiceProperty

	hr, _, _ := syscall.Syscall(
		v.VTable().GetVoiceProperty,
		3,
		uintptr(unsafe.Pointer(v)),
		uintptr(index),
		uintptr(unsafe.Pointer(&property)))

	if hr != 0 {
		return nil, ole.NewError(hr)
	}

	result := &types.VoiceProperty{
		Language:     LPWSTRToString(property.Language),
		DisplayName:  LPWSTRToString(property.DisplayName),
		SpeakingRate: property.SpeakingRate,
		Volume:       property.Volume,
		Pitch:        property.Pitch,
	}

	ole.CoTaskMemFree(property.Language)
	ole.CoTaskMemFree(property.DisplayName)
	ole.CoTaskMemFree(uintptr(unsafe.Pointer(property)))

	return result, nil
}

func asSetDefaultVoice(v *IAudioServer, index int32) error {
	hr, _, _ := syscall.Syscall(
		v.VTable().SetDefaultVoice,
		2,
		uintptr(unsafe.Pointer(v)),
		uintptr(index),
		0)

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}

func asSetVoiceProperty(v *IAudioServer, index int32, property *types.VoiceProperty) error {
	p := types.RawVoiceProperty{
		SpeakingRate: property.SpeakingRate,
		Pitch:        property.Pitch,
		Volume:       property.Volume,
	}
	log.Printf("@@@ %+v\n", p)
	hr, _, _ := syscall.Syscall(
		v.VTable().SetVoiceProperty,
		3,
		uintptr(unsafe.Pointer(v)),
		uintptr(index),
		uintptr(unsafe.Pointer(&p)))

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}

func asSetNotifyIdleStateHandler(v *IAudioServer, handler NotifyIdleStateFunc) error {
	hr, _, _ := syscall.Syscall(
		v.VTable().SetNotifyIdleStateHandler,
		2,
		uintptr(unsafe.Pointer(v)),
		uintptr(syscall.NewCallback(handler)),
		0)

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}

func LPWSTRToString(lpwstr uintptr) string {
	if lpwstr == 0 {
		return ""
	}

	us := []uint16{}

	for i := 0; i < 1024; i += 2 {
		u := *(*uint16)(unsafe.Pointer(lpwstr + uintptr(i)))

		if u == 0 {
			break
		}

		us = append(us, u)
	}

	return syscall.UTF16ToString(us)
}
