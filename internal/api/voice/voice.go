package voice

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

type GetDefaultVoicePropertyRes struct {
	Count                int32                `json:"count"`
	DefaultVoiceProperty *types.VoiceProperty `json:"defaultVoiceProperty"`
}

func getDefaultVoice(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	count, err := core.GetVoiceCount()

	if err != nil {
		response := fmt.Sprintf("{\"error\": \"%s\"}", err)

		log.Println(err)
		http.Error(w, response, http.StatusBadRequest)

	}

	property, err := core.GetDefaultVoiceProperty()

	if err != nil {
		response := fmt.Sprintf("{\"error\": \"%s\"}", err)

		log.Println(err)
		http.Error(w, response, http.StatusBadRequest)

	}

	data, err := json.Marshal(GetDefaultVoicePropertyRes{
		Count:                count,
		DefaultVoiceProperty: property,
	})

	if err != nil {
		response := fmt.Sprintf("{\"error\": \"%s\"}", err)

		log.Println(err)
		http.Error(w, response, http.StatusBadRequest)

		return
	}
	if _, err := io.Copy(w, bytes.NewBuffer(data)); err != nil {
		response := fmt.Sprintf("{\"error\": \"%s\"}", err)

		log.Println(err)
		http.Error(w, response, http.StatusBadRequest)

		return
	}
}

func putDefaultVoice(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	buffer := &bytes.Buffer{}

	if _, err := io.Copy(buffer, r.Body); err != nil {
		response := fmt.Sprintf("{\"error\": \"%s\"}", err)

		log.Println(err)
		http.Error(w, response, http.StatusBadRequest)

		return
	}

	var property types.VoiceProperty

	if err := json.Unmarshal(buffer.Bytes(), &property); err != nil {
		response := fmt.Sprintf("{\"error\": \"%s\"}", err)

		log.Println(err)
		http.Error(w, response, http.StatusBadRequest)

		return
	}
	if err := core.SetDefaultVoiceProperty(&property); err != nil {
		response := fmt.Sprintf("{\"error\": \"%s\"}", err)

		log.Println(err)
		http.Error(w, response, http.StatusBadRequest)

		return
	}
	if _, err := io.WriteString(w, `{"success":true}`); err != nil {
		response := fmt.Sprintf("{\"error\": \"%s\"}", err)

		log.Println(err)
		http.Error(w, response, http.StatusBadRequest)

		return
	}
}

func getVoiceProperty(w http.ResponseWriter, r *http.Request) {
	log.Println("getVoiceProperty", r.Method, r.URL)

	return
}

func putVoiceProperty(w http.ResponseWriter, r *http.Request) {
	log.Println("putVoiceProperty", r.Method, r.URL)

	return
}
