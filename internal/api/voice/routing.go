package voice

import "github.com/go-chi/chi"

func Setup(router chi.Router) {
	router.Route("/voice", func(r chi.Router) {
		r.Get("/", getVoices)
		r.Get("/{voiceId}", getVoiceProperty)
		r.Put("/{voiceId}", putVoiceProperty)
	})
}
