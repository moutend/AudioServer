package voice

import (
	"log"
	"net/http"
)

func getDefaultVoice(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	return
}

func putDefaultVoice(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	return
}

func getVoiceProperty(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	return
}

func putVoiceProperty(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	return
}
