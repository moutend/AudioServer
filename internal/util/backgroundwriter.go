package util

import (
	"os"
	"path/filepath"
	"sync"
)

type BackgroundWriter struct {
	dataChan   chan []byte
	quitChan   chan struct{}
	wg         *sync.WaitGroup
	outputPath string
}

func (bw *BackgroundWriter) Close() {
	bw.quitChan <- struct{}{}

	bw.wg.Wait()
}

func (bw *BackgroundWriter) Write(p []byte) (n int, err error) {
	b := make([]byte, len(p))
	copy(b, p)
	bw.dataChan <- b

	return len(p), nil
}

func NewBackgroundWriter(outputPath string) *BackgroundWriter {
	dataChan := make(chan []byte)
	quitChan := make(chan struct{})
	wg := &sync.WaitGroup{}

	wg.Add(1)

	go func() {
		defer wg.Done()

		for {
			select {
			case p := <-dataChan:
				os.MkdirAll(filepath.Dir(outputPath), 0755)

				file, err := os.OpenFile(outputPath, os.O_RDWR|os.O_APPEND|os.O_CREATE, 0644)

				if err != nil {
					continue
				}
				if n, err := file.Write(p); err != nil || n != len(p) {
					continue
				}

				file.Close()
			case <-quitChan:
				return
			}
		}
	}()

	return &BackgroundWriter{dataChan, quitChan, wg, outputPath}
}
