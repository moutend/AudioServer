package api

import (
	"github.com/go-chi/chi"
	"github.com/moutend/AudioServer/internal/api/audio"
	"github.com/moutend/AudioServer/internal/api/voice"
)

func Setup(router chi.Router) {
	router.Route("/v1", func(r chi.Router) {
		audio.Setup(r)
		voice.Setup(r)
	})
}
