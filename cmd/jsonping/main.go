package main

import (
	"bytes"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
)

func main() {
	log.SetFlags(0)
	log.SetPrefix("error: ")

	if err := run(); err != nil {
		log.Fatal(err)
	}
}

func run() (err error) {
	var (
		pathFlag   string
		methodFlag string
	)

	flag.StringVar(&pathFlag, "path", "", "Path to the JSON file.")
	flag.StringVar(&methodFlag, "method", "", "HTTP method e.g. POST")
	flag.Parse()

	if len(flag.Args()) < 1 {
		return fmt.Errorf("specify address")
	}

	address := flag.Args()[0]
	client := &http.Client{}

	var req *http.Request

	if pathFlag == "" {
		req, err = http.NewRequest(methodFlag, address, nil)
	} else {
		data, err := ioutil.ReadFile(pathFlag)

		if err != nil {
			return err
		}

		req, err = http.NewRequest(methodFlag, address, bytes.NewBuffer(data))
	}
	if err != nil {
		return err
	}

	res, err := client.Do(req)

	if err != nil {
		return err
	}

	defer res.Body.Close()

	body := &bytes.Buffer{}

	if _, err := io.Copy(body, res.Body); err != nil {
		return err
	}

	fmt.Printf("%v %v\n", res.Status, body.Bytes())

	return nil
}
