package com

import (
	"unsafe"

	"github.com/go-ole/go-ole"
)

type IAudioServer struct {
	ole.IUnknown
	ole.IDispatch
}

type IAudioServerVtbl struct {
	ole.IUnknownVtbl
	ole.IDispatchVtbl
	Start            uintptr
	Stop             uintptr
	FadeIn           uintptr
	FadeOut          uintptr
	Push             uintptr
	GetVoiceCount    uintptr
	GetDefaultVoice  uintptr
	GetVoiceProperty uintptr
	SetDefaultVoice  uintptr
	SetVoiceProperty uintptr
}

func (v *IAudioServer) VTable() *IAudioServerVtbl {
	return (*IAudioServerVtbl)(unsafe.Pointer(v.RawVTable))
}

func (v *IAudioServer) Start() error {
	return asStart(v)
}

func (v *IAudioServer) Stop() error {
	return asStop(v)
}

func (v *IAudioServer) FadeIn() error {
	return asFadeIn(v)
}

func (v *IAudioServer) FadeOut() error {
	return asFadeOut(v)
}
