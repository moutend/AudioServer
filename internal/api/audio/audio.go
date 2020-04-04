package audio

import (
	"log"
	"net/http"
)

func postAudio(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	return
}

func postAudioRestart(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	return
}

func postAudioPause(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	return
}
