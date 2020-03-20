// +build windows

package com

import (
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
		v.VTable().FadeOut,
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
