package mux

import (
	"bytes"
	"fmt"
	"io"
	"net/http"
)

type Mux struct {
	patterns map[string]http.HandlerFunc
}

func getKey(method, path string) string {
	return method + " " + path
}

func (m *Mux) Get(path string, fn http.HandlerFunc) {
	m.patterns[getKey(http.MethodGet, path)] = fn
}

func (m *Mux) Put(path string, fn http.HandlerFunc) {
	m.patterns[getKey(http.MethodPut, path)] = fn
}

func (m *Mux) Post(path string, fn http.HandlerFunc) {
	m.patterns[getKey(http.MethodPost, path)] = fn
}

func (m *Mux) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	var err error

	defer func() {
		if err == nil {
			return
		}

		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusBadRequest)

		data := fmt.Sprintf(`{"error":%q}`, err)
		io.Copy(w, bytes.NewBufferString(data))
	}()

	if fn, ok := m.patterns[getKey(r.Method, r.URL.Path)]; ok {
		fn(w, r)
	} else {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusNotFound)

		_, err = io.Copy(w, bytes.NewBufferString(`{"error":"404 Not found"}`))
	}
}

func New() *Mux {
	m := &Mux{
		patterns: make(map[string]http.HandlerFunc),
	}

	return m
}
