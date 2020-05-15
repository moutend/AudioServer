package audio

import (
	"bytes"
	"encoding/json"
	"io"
	"log"
	"net/http"
	"time"

	"github.com/moutend/AudioServer/internal/core"

	"github.com/moutend/AudioServer/pkg/types"
)

type PostAudioReq struct {
	IsForcePush bool            `json:"isForcePush"`
	Commands    []types.Command `json:"commands"`
}
var count int
func postAudio(w http.ResponseWriter, r *http.Request) {
count += 1
	now := time.Now().UTC()
	log.Println(count, r.Method, r.URL, now.Unix(), now.Nanosecond())
return
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

func postAudioRestart(w http.ResponseWriter, r *http.Request) {
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

func postAudioPause(w http.ResponseWriter, r *http.Request) {
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
