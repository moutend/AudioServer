package api

import (
	"github.com/go-chi/chi"
	"github.com/moutend/AudioServer/internal/api/audio"
	"github.com/moutend/AudioServer/internal/api/voice"
)

func Setup(router chi.Router) chi.Router {
	return router.Route("/v1", func(r chi.Router) {
		r = audio.Setup(r)
		r = voice.Setup(r)
	})
}
