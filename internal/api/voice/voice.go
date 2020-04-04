package voice

import (
	"log"
	"net/http"
)

func getDefaultVoice(w http.ResponseWriter, r *http.Request) {
	log.Println("getDefaultVoice", r.Method, r.URL)

	return
}

func putDefaultVoice(w http.ResponseWriter, r *http.Request) {
	log.Println("putDefaultVoice", r.Method, r.URL)

	return
}

func getVoiceProperty(w http.ResponseWriter, r *http.Request) {
	log.Println("getVoiceProperty", r.Method, r.URL)

	return
}

func putVoiceProperty(w http.ResponseWriter, r *http.Request) {
	log.Println("putVoiceProperty", r.Method, r.URL)

	return
}
