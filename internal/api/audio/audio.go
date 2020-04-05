package audio

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
	IsForcePush bool      `json:"isForcePush"`
	Commands    []Command `json:"commands"`
}

type Command struct {
	Type  int16       `json:"type"`
	Value interface{} `json:"value"`
}

func postAudio(w http.ResponseWriter, r *http.Request) {
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

	commands := []types.Command{}

	for _, v := range req.Commands {
		switch v.Type {
		case 1:
			if value, ok := v.Value.(float64); ok {
				commands = append(commands, types.Command{
					Type:     v.Type,
					SFXIndex: int16(value),
				})
			}
		case 2:
			if value, ok := v.Value.(float64); ok {
				commands = append(commands, types.Command{
					Type:         v.Type,
					WaitDuration: value,
				})
			}
		case 3, 4:
			if value, ok := v.Value.(string); ok {
				commands = append(commands, types.Command{
					Type:         v.Type,
					TextToSpeech: value,
				})
			}
		}
	}
	if err := core.Push(req.IsForcePush, commands); err != nil {
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
