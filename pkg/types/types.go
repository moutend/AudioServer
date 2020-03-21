package types

type Command struct {
	Type         int16
	SFXIndex     int16
	TextToSpeech string
	WaitDuration float64
}

type RawCommand struct {
	Type         int16
	SFXIndex     int16
	TextToSpeech uintptr
	WaitDuration float64
}

type VoiceProperty struct {
	Id           int
	Language     string
	DisplayName  string
	SpeakingRate float64
	Volume       float64
	Pitch        float64
}

type RawVoiceProperty struct {
	Id                int32
	LanguageLength    int16
	DisplayNameLength int16
	Language          uintptr
	DisplayName       uintptr
	SpeakingRate      float64
	Volume            float64
	Pitch             float64
}
