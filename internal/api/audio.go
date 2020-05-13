package api

import (
	"bytes"
	"encoding/json"
	"io"
	"log"
	"net/http"

	"github.com/moutend/AudioServer/internal/core"

	"github.com/moutend/AudioServer/pkg/types"
)

type PostAudioReq struct {
	IsForcePush bool            `json:"isForcePush"`
	Commands    []types.Command `json:"commands"`
}

func PostAudio(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	w.Header().Set("Content-Type", "application/json")

	buffer := &bytes.Buffer{}

	if _, err := io.Copy(buffer, r.Body); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	var req PostAudioReq

	if err := json.Unmarshal(buffer.Bytes(), &req); err != nil {
		response := "{\"error\": \"%s\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
	if err := core.Push(req.IsForcePush, req.Commands); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
	if _, err := io.WriteString(w, `{"success": true}`); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
}

func PostAudioRestart(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	w.Header().Set("Content-Type", "application/json")

	if err := core.FadeIn(); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
	if _, err := io.WriteString(w, `{"success": true}`); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
}

func PostAudioPause(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	w.Header().Set("Content-Type", "application/json")

	if err := core.FadeOut(); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
	if _, err := io.WriteString(w, `{"success": true}`); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
}
