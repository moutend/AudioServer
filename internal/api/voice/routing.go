package voice

import "github.com/go-chi/chi"

func Setup(router chi.Router) {
	router.Route("/voice", func(r chi.Router) {
		r.Get("/", getVoices)

		r.Post("/default/pitch/up", postDefaultVoicePitchUp)
		r.Post("/default/pitch/down", postDefaultVoicePitchDown)
		r.Post("/default/rate/up", postDefaultVoiceRateUp)
		r.Post("/default/rate/down", postDefaultVoiceRateDown)
		r.Post("/default/volume/up", postDefaultVoiceVolumeUp)
		r.Post("/default/volume/down", postDefaultVoiceVolumeDown)
		r.Get("/default", getDefaultVoiceProperty)
		r.Put("/default", putDefaultVoiceProperty)

		r.Get("/{voiceId}", getVoiceProperty)
		r.Put("/{voiceId}", putVoiceProperty)
	})
}
