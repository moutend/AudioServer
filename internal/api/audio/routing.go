package audio

import "github.com/go-chi/chi"

func Setup(router chi.Router) chi.Router {
	return router.Route("/audio", func(r chi.Router) {
		r.Post("", postAudio)
		r.Post("/restart", postAudioRestart)
		r.Post("/pause", postAudioPause)
	})
}
