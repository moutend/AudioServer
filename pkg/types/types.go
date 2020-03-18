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
