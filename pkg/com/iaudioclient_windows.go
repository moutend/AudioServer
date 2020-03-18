// +build windows

package com

import (
	"syscall"

	"github.com/go-ole/go-ole"
)

func asStart(v *IAudioServer) error {
	hr, _, _ := syscall.Syscall(v.VTable().Start, 0, 0, 0, 0)

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}

func asStop(v *IAudioServer) error {
	hr, _, _ := syscall.Syscall(v.VTable().Stop, 0, 0, 0, 0)

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}

func asFadeIn(v *IAudioServer) error {
	hr, _, _ := syscall.Syscall(v.VTable().FadeIn, 0, 0, 0, 0)

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}

func asFadeOut(v *IAudioServer) error {
	hr, _, _ := syscall.Syscall(v.VTable().FadeOut, 0, 0, 0, 0)

	if hr != 0 {
		return ole.NewError(hr)
	}

	return nil
}
