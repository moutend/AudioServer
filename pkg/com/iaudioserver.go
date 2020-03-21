package com

import (
	"unsafe"

	"github.com/go-ole/go-ole"
	"github.com/moutend/AudioServer/pkg/types"
)

type IAudioServer struct {
	ole.IDispatch
}

type IAudioServerVtbl struct {
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

func (v *IAudioServer) Push(commands []types.Command) error {
	return asPush(v, commands)
}

func (v *IAudioServer) GetVoiceCount() (int, error) {
	return asGetVoiceCount(v)
}

func (v *IAudioServer) GetDefaultVoice() (int, error) {
	return asGetDefaultVoice(v)
}

func (v *IAudioServer) GetVoiceProperty(index int) (*types.VoiceProperty, error) {
	return asGetVoiceProperty(v, index)
}
