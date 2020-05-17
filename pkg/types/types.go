package types

type Command struct {
	Type          int16   `json:"type"`
	SFXIndex      int16   `json:"sfxIndex"`
	TextToSpeech  string  `json:"textToSpeech"`
	SleepDuration float64 `json:"sleepDuration"`
	Pan           float64 `json:"pan"`
}

type RawCommand struct {
	Type          int16
	SFXIndex      int16
	TextToSpeech  uintptr
	SleepDuration float64
	Pan           float64
}

type VoiceProperty struct {
	Language     string
	DisplayName  string
	SpeakingRate float64
	Volume       float64
	Pitch        float64
}

type RawVoiceProperty struct {
	Language     uintptr
	DisplayName  uintptr
	SpeakingRate float64
	Volume       float64
	Pitch        float64
}
