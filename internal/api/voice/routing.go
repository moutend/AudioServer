package voice

import "github.com/go-chi/chi"

func Setup(router chi.Router) {
	router.Route("/voice", func(r chi.Router) {
		r.Get("/", getVoices)

		r.Post("/voice/pitch/up", postDefaultVoicePitchUp)
		r.Post("/voice/pitch/down", postDefaultVoicePitchDown)
		r.Post("/voice/rate/up", postDefaultVoiceRateUp)
		r.Post("/voice/rate/down", postDefaultVoiceRateDown)
		r.Post("/voice/volume/up", postDefaultVoiceVolumeUp)
		r.Post("/voice/volume/down", postDefaultVoiceVolumeDown)

		r.Get("/", getDefaultVoiceProperty)
		r.Put("/", putDefaultVoiceProperty)

		r.Get("/{voiceId}", getVoiceProperty)
		r.Put("/{voiceId}", putVoiceProperty)
	})
}
