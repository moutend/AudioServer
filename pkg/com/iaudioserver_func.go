// +build !windows

package com

import (
	"github.com/go-ole/go-ole"
	"github.com/moutend/AudioServer/pkg/types"
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

func asPush(v *IAudioServer, isForcePush bool, commands []types.Command) error {
	return ole.NewError(ole.E_NOTIMPL)
}

func asGetVoiceCount(v *IAudioServer) (int, error) {
	return -1, ole.NewError(ole.E_NOTIMPL)
}

func asGetDefaultVoice(v *IAudioServer) (int, error) {
	return -1, ole.NewError(ole.E_NOTIMPL)
}

func asGetVoiceProperty(v *IAudioServer, index int) (*types.VoiceProperty, error) {
	return nil, ole.NewError(ole.E_NOTIMPL)
}

func asSetDefaultVoice(v *IAudioServer, index int) error {
	return ole.NewError(ole.E_NOTIMPL)
}

func asSetVoiceProperty(v *IAudioServer, index int, property *types.VoiceProperty) error {
	return ole.NewError(ole.E_NOTIMPL)
}

func asSetNotifyIdleStateHandler(v *IAudioServer, handler NotifyIdleStateFunc) error {
	return ole.NewError(ole.E_NOTIMPL)
}
