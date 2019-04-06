package main

import (
	"bufio"
	"github.com/tarm/serial"
	"io"
	"log"
	"os"
	"regexp"
	"strconv"
	"strings"
)

const (
	obsFile = `C:\Users\sven\Documents\stream\heartrate\heartrate.txt`
	comPort = `COM9`
	debug   = false
)

var (
	connectedRegex = regexp.MustCompile(`connected: (\d+)`)
	rateRegex      = regexp.MustCompile(`rate: (\d+)`)
)

func main() {

	hrChan := make(chan int)
	go startSerialMagic(hrChan)

	file, err := os.Create(obsFile)
	if err != nil {
		panic(err)
	}

	for hr := range hrChan {
		file.Truncate(0)
		file.Seek(0, io.SeekStart)

		if _, err = file.WriteString(strconv.Itoa(hr)); err != nil {
			panic(err)
		}
	}

}

func startSerialMagic(rateChan chan int) {
	c := &serial.Config{Name: comPort, Baud: 115200}
	s, err := serial.OpenPort(c)
	if err != nil {
		log.Fatal(err)
	}

	scanner := bufio.NewScanner(s)
	for scanner.Scan() {
		text := scanner.Text()
		handleInput(text, rateChan)
	}
}

func handleInput(text string, rateChan chan int) {
	// skip parsing the data if we don't have the prefix
	if !strings.HasPrefix(text, "d:") {
		if debug {
			log.Println(text)
		}

		return
	}

	if matches := connectedRegex.FindStringSubmatch(text); len(matches) > 0 {
		log.Printf("connected: %t", matches[1] == "1")
		return
	}

	if matches := rateRegex.FindStringSubmatch(text); len(matches) > 0 {
		hrString := matches[1]
		heartRate, err := strconv.Atoi(hrString)
		if err != nil {
			log.Printf("error reading hearrate %s: %v", hrString, err)

		}
		log.Printf("actual heartrate %d", heartRate)

		select {
		case rateChan <- heartRate:
		default:
			log.Println("dropping HR event because channel is busy")
		}
		return
	}
}
