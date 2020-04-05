package types

type Command struct {
	Type         int16   `json:"type"`
	SFXIndex     int16   `json:"sfxIndex"`
	TextToSpeech string  `json:"textToSpeech"`
	WaitDuration float64 `json:"waitDuration"`
}

type RawCommand struct {
	Type         int16
	SFXIndex     int16
	TextToSpeech uintptr
	WaitDuration float64
}

type VoiceProperty struct {
	Id           int32   `json:"id"`
	Language     string  `json:"language"`
	DisplayName  string  `json:"displayName"`
	SpeakingRate float64 `json:"speakingRate"`
	Volume       float64 `json:"volume"`
	Pitch        float64 `json:"pitch"`
}

type RawVoiceProperty struct {
	Language     uintptr
	DisplayName  uintptr
	SpeakingRate float64
	Volume       float64
	Pitch        float64
}
