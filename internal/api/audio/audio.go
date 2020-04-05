package audio

import (
	"bytes"
	"encoding/json"
	"fmt"
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

	buffer := &bytes.Buffer{}

	if _, err := io.Copy(buffer, r.Body); err != nil {
		response := fmt.Sprintf("{\"error\": \"%s\"}", err)

		log.Println(err)
		http.Error(w, response, http.StatusBadRequest)

		return
	}

	var req PostAudioReq

	if err := json.Unmarshal(buffer.Bytes(), &req); err != nil {
		response := fmt.Sprintf("{\"error\": \"%s\"}", err)

		log.Println(err)
		http.Error(w, response, http.StatusBadRequest)

		return
	}

	commands := make([]types.Command, len(req.Commands))

	for i, v := range req.Commands {
		commands[i].Type = v.Type

		switch v.Type {
		case 1:
			commands[i].SFXIndex = v.Value.(int16)
		case 2:
			commands[i].WaitDuration = v.Value.(float64)
		case 3, 4:
			commands[i].TextToSpeech = v.Value.(string)
		}
	}
	if err := core.Push(req.IsForcePush, commands); err != nil {
		response := fmt.Sprintf("{\"error\": \"%s\"}", err)

		log.Println(err)
		http.Error(w, response, http.StatusBadRequest)

		return
	}
	if _, err := io.WriteString(w, `{"success": true}`); err != nil {
		log.Println(err)
	}
}

func postAudioRestart(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	if err := core.FadeIn(); err != nil {
		response := fmt.Sprintf("{\"error\": \"%s\"}", err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
}

func postAudioPause(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	if err := core.FadeOut(); err != nil {
		response := fmt.Sprintf("{\"error\": \"%s\"}", err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
}
