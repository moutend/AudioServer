package audio

import (
	"log"
	"net/http"
)

func postAudio(w http.ResponseWriter, r *http.Request) {
	log.Println("postAudio", r.Method, r.URL)

	return
}

func postAudioRestart(w http.ResponseWriter, r *http.Request) {
	log.Println("postAudioRestart", r.Method, r.URL)

	return
}

func postAudioPause(w http.ResponseWriter, r *http.Request) {
	log.Println("postAudioPause", r.Method, r.URL)

	return
}
