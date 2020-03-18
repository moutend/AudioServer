// +build !windows

package com

import (
	"github.com/go-ole/go-ole"
)

func asStart(v *IAudioServer) error {
	return ole.NewError(ole.E_NOTIMPL)
}

func asStop(v *IAudioServer) error {
	return ole.NewError(ole.E_NOTIMPL)
}

func asFadeIn(v *IAudioServer) error {
	return ole.NewError(ole.E_NOTIMPL)
}

func asFadeOut(v *IAudioServer) error {
	return ole.NewError(ole.E_NOTIMPL)
}
