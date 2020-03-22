// +build windows

package com

import (
	"reflect"
	"syscall"
	"unsafe"

	"github.com/moutend/AudioServer/pkg/types"

	"github.com/go-ole/go-ole"
)

func asStart(v *IAudioServer) error {
	hr, _, _ := syscall.Syscall(
		v.VTable().Start,
		0,
		uintptr(unsafe.Pointer(v)),
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
		0,
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
		0,
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
		0,
		uintptr(unsafe.Pointer(v)),
		0,
		0)

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}

func asPush(v *IAudioServer, commands []types.Command) error {
	cs := make([]types.RawCommand, len(commands), len(commands))
	ps := make([]uintptr, len(commands), len(commands))

	for i, c := range commands {
		cs[i] = types.RawCommand{
			Type:         c.Type,
			SFXIndex:     c.SFXIndex,
			WaitDuration: c.WaitDuration,
		}
		if c.Type == 3 {
			u16ptr, err := syscall.UTF16PtrFromString(c.TextToSpeech)

			if err != nil {
				return err
			}

			cs[i].TextToSpeech = uintptr(unsafe.Pointer(u16ptr))

			ps[i] = uintptr(unsafe.Pointer(&cs[i]))
		}
	}

	hr, _, _ := syscall.Syscall6(
		v.VTable().Push,
		3,
		uintptr(unsafe.Pointer(v)),
		uintptr(unsafe.Pointer(&ps[0])),
		uintptr(len(commands)),
		1,
		0,
		0)

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}

func asGetVoiceCount(v *IAudioServer) (int, error) {
	count := int32(0)

	hr, _, _ := syscall.Syscall(
		v.VTable().GetVoiceCount,
		1,
		uintptr(unsafe.Pointer(v)),
		uintptr(unsafe.Pointer(&count)),
		0)

	if hr != 0 {
		return -1, ole.NewError(hr)
	}

	return int(count), nil
}

func asGetDefaultVoice(v *IAudioServer) (int, error) {
	index := int32(0)

	hr, _, _ := syscall.Syscall(
		v.VTable().GetDefaultVoice,
		1,
		uintptr(unsafe.Pointer(v)),
		uintptr(unsafe.Pointer(&index)),
		0)

	if hr != 0 {
		return -1, ole.NewError(hr)
	}

	return int(index), nil
}

func asGetVoiceProperty(v *IAudioServer, index int) (*types.VoiceProperty, error) {
	property := &types.RawVoiceProperty{}

	hr, _, _ := syscall.Syscall(
		v.VTable().GetVoiceProperty,
		2,
		uintptr(unsafe.Pointer(v)),
		uintptr(index),
		uintptr(unsafe.Pointer(&property)))

	if hr != 0 {
		return nil, ole.NewError(hr)
	}

	result := &types.VoiceProperty{
		Id: int(property.Id),
		// Language:     u16ptrToString(property.Language, int(property.LanguageLength)),
		// DisplayName:  u16ptrToString(property.DisplayName, int(property.DisplayNameLength)),
		SpeakingRate: property.SpeakingRate,
		Volume:       property.Volume,
		Pitch:        property.Pitch,
	}

	return result, nil
}

func u16ptrToString(data uintptr, capacity int) string {
	if data == 0 || capacity == 0 {
		return ""
	}

	u16hdr := reflect.SliceHeader{Data: data, Len: capacity, Cap: capacity}
	u16s := *(*[]uint16)(unsafe.Pointer(&u16hdr))

	return syscall.UTF16ToString(u16s)
}
