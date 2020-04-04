package util

import (
	"fmt"
	"io/ioutil"
	"log"
	"math/rand"
	"os"
	"testing"
	"time"
)

func TestBackgroundWriter(t *testing.T) {
	rand.Seed(time.Now().Unix())

	filename := fmt.Sprintf("output-%d.txt", rand.Int())
	defer os.Remove(filename)

	bw := NewBackgroundWriter(filename)

	log.SetOutput(bw)
	log.SetFlags(0)

	log.Printf("foo\n")
	log.Printf("bar\n")
	log.Printf("2000\n")

	bw.Close()

	result, err := ioutil.ReadFile(filename)

	if err != nil {
		t.Fatal(err)
	}

	actual := fmt.Sprintf("%s", result)
	expected := "foo\nbar\n2000\n"

	if actual != expected {
		t.Fatalf("\nactual: %q\nexpected: %q", actual, expected)
	}
}
