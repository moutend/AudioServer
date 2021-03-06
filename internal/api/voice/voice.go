package voice

import (
	"bytes"
	"encoding/json"
	"io"
	"log"
	"net/http"
	"strconv"

	"github.com/go-chi/chi"
	"github.com/moutend/AudioServer/internal/core"
)

type VoiceProperty struct {
	Id             int32   `json:"id"`
	Language       string  `json:"language"`
	DisplayName    string  `json:"displayName"`
	SpeakingRate   float64 `json:"speakingRate"`
	Pitch          float64 `json:"pitch"`
	Volume         float64 `json:"volume"`
	IsDefaultVoice bool    `json:"isDefaultVoice"`
}

type GetVoicesResponse struct {
	Voices []VoiceProperty `json:"voices"`
}

type GetVoiceResponse VoiceProperty

type PutVoiceRequest struct {
	SpeakingRate float64 `json:"speakingRate"`
	Pitch        float64 `json:"pitch"`
	Volume       float64 `json:"volume"`
}

func getVoices(w http.ResponseWriter, r *http.Request) {
	log.Println(r.Method, r.URL)

	w.Header().Set("Content-Type", "application/json")

	count, err := core.GetVoiceCount()

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	defaultVoiceIndex, err := core.GetDefaultVoice()

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	voices := make([]VoiceProperty, count)

	for i := int32(0); i < count; i++ {
		property, err := core.GetVoiceProperty(i)

		if err != nil {
			response := "{\"error\": \"internal error\"}"

			log.Println(err)
			http.Error(w, response, http.StatusInternalServerError)

			return
		}

		voices[i] = VoiceProperty{
			Id:             i,
			Language:       property.Language,
			DisplayName:    property.DisplayName,
			SpeakingRate:   property.SpeakingRate,
			Pitch:          property.Pitch,
			Volume:         property.Volume,
			IsDefaultVoice: i == defaultVoiceIndex,
		}
	}

	data, err := json.Marshal(GetVoicesResponse{
		Voices: voices,
	})

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
	if _, err := io.Copy(w, bytes.NewBuffer(data)); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
}

func postDefaultVoiceVolumeUp(w http.ResponseWriter, r *http.Request) {
	log.Println("postDefaultVoiceVolumeUp")

	w.Header().Set("Content-Type", "application/json")

	defaultVoiceIndex, err := core.GetDefaultVoice()

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property, err := core.GetVoiceProperty(int32(defaultVoiceIndex))

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property.Volume += 0.01

	if err := core.SetVoiceProperty(int32(defaultVoiceIndex), property); err != nil {
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

func postDefaultVoiceVolumeDown(w http.ResponseWriter, r *http.Request) {
	log.Println("postDefaultVoiceVolumeDown")

	w.Header().Set("Content-Type", "application/json")

	defaultVoiceIndex, err := core.GetDefaultVoice()

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property, err := core.GetVoiceProperty(int32(defaultVoiceIndex))

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property.Volume -= 0.01

	if err := core.SetVoiceProperty(int32(defaultVoiceIndex), property); err != nil {
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

func postDefaultVoiceRateUp(w http.ResponseWriter, r *http.Request) {
	log.Println("postDefaultVoiceRateUp")

	w.Header().Set("Content-Type", "application/json")

	defaultVoiceIndex, err := core.GetDefaultVoice()

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property, err := core.GetVoiceProperty(int32(defaultVoiceIndex))

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property.SpeakingRate += 0.01

	if err := core.SetVoiceProperty(int32(defaultVoiceIndex), property); err != nil {
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

func postDefaultVoiceRateDown(w http.ResponseWriter, r *http.Request) {
	log.Println("postDefaultVoiceRateDown")

	w.Header().Set("Content-Type", "application/json")

	defaultVoiceIndex, err := core.GetDefaultVoice()

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property, err := core.GetVoiceProperty(int32(defaultVoiceIndex))

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property.SpeakingRate -= 0.01

	if err := core.SetVoiceProperty(int32(defaultVoiceIndex), property); err != nil {
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

func postDefaultVoicePitchUp(w http.ResponseWriter, r *http.Request) {
	log.Println("postDefaultVoicePitchUp")

	w.Header().Set("Content-Type", "application/json")

	defaultVoiceIndex, err := core.GetDefaultVoice()

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property, err := core.GetVoiceProperty(int32(defaultVoiceIndex))

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property.Pitch += 0.01

	if err := core.SetVoiceProperty(int32(defaultVoiceIndex), property); err != nil {
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

func postDefaultVoicePitchDown(w http.ResponseWriter, r *http.Request) {
	log.Println("postDefaultVoicePitchDown")

	w.Header().Set("Content-Type", "application/json")

	defaultVoiceIndex, err := core.GetDefaultVoice()

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property, err := core.GetVoiceProperty(int32(defaultVoiceIndex))

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property.Pitch -= 0.01

	if err := core.SetVoiceProperty(int32(defaultVoiceIndex), property); err != nil {
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

func getDefaultVoiceProperty(w http.ResponseWriter, r *http.Request) {
	log.Println("getDefaultVoiceProperty", r.Method, r.URL)

	w.Header().Set("Content-Type", "application/json")

	defaultVoiceIndex, err := core.GetDefaultVoice()

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property, err := core.GetVoiceProperty(int32(defaultVoiceIndex))

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	data, err := json.Marshal(GetVoiceResponse{
		Id:             int32(defaultVoiceIndex),
		Language:       property.Language,
		DisplayName:    property.DisplayName,
		SpeakingRate:   property.SpeakingRate,
		Pitch:          property.Pitch,
		Volume:         property.Volume,
		IsDefaultVoice: true,
	})

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
	if _, err := io.Copy(w, bytes.NewBuffer(data)); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
}

func putDefaultVoiceProperty(w http.ResponseWriter, r *http.Request) {
	log.Println("putDefaultVoiceProperty", r.Method, r.URL)

	w.Header().Set("Content-Type", "application/json")

	buffer := &bytes.Buffer{}

	if _, err := io.Copy(buffer, r.Body); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	var req PutVoiceRequest

	if err := json.Unmarshal(buffer.Bytes(), &req); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	defaultVoiceIndex, err := core.GetDefaultVoice()

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property, err := core.GetVoiceProperty(int32(defaultVoiceIndex))

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property.SpeakingRate = req.SpeakingRate
	property.Pitch = req.Pitch
	property.Volume = property.Volume

	if err := core.SetVoiceProperty(int32(defaultVoiceIndex), property); err != nil {
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

func getVoiceProperty(w http.ResponseWriter, r *http.Request) {
	log.Println("getVoiceProperty", r.Method, r.URL)

	w.Header().Set("Content-Type", "application/json")

	voiceId := chi.URLParam(r, "voiceId")

	if voiceId == "" {
		response := "{\"error\": \"specify voice id\"}"

		log.Println("API called without voice id")
		http.Error(w, response, http.StatusBadRequest)

		return
	}

	index, err := strconv.Atoi(voiceId)

	if err != nil {
		response := "{\"error\": \"invalid voice id\"}"

		log.Println(err)
		http.Error(w, response, http.StatusBadRequest)

		return
	}

	defaultVoiceIndex, err := core.GetDefaultVoice()

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property, err := core.GetVoiceProperty(int32(index))

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	data, err := json.Marshal(GetVoiceResponse{
		Id:             int32(index),
		Language:       property.Language,
		DisplayName:    property.DisplayName,
		SpeakingRate:   property.SpeakingRate,
		Pitch:          property.Pitch,
		Volume:         property.Volume,
		IsDefaultVoice: int32(index) == defaultVoiceIndex,
	})

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
	if _, err := io.Copy(w, bytes.NewBuffer(data)); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}
}

func putVoiceProperty(w http.ResponseWriter, r *http.Request) {
	log.Println("putVoiceProperty", r.Method, r.URL)

	w.Header().Set("Content-Type", "application/json")

	voiceId := chi.URLParam(r, "voiceId")

	if voiceId == "" {
		response := "{\"error\": \"specify voice id\"}"

		log.Println("API called without voice id")
		http.Error(w, response, http.StatusBadRequest)

		return
	}

	index, err := strconv.Atoi(voiceId)

	if err != nil {
		response := "{\"error\": \"invalid voice id\"}"

		log.Println(err)
		http.Error(w, response, http.StatusBadRequest)

		return
	}

	buffer := &bytes.Buffer{}

	if _, err := io.Copy(buffer, r.Body); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	var req PutVoiceRequest

	if err := json.Unmarshal(buffer.Bytes(), &req); err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property, err := core.GetVoiceProperty(int32(index))

	if err != nil {
		response := "{\"error\": \"internal error\"}"

		log.Println(err)
		http.Error(w, response, http.StatusInternalServerError)

		return
	}

	property.SpeakingRate = req.SpeakingRate
	property.Pitch = req.Pitch
	property.Volume = property.Volume

	if err := core.SetVoiceProperty(int32(index), property); err != nil {
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
