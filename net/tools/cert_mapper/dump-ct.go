// This tool maps over a certificate database file (as written and read by
// github.com/agl/certificatetransparency), and dumps all the certificates to a
// file.
//
// The file is structured simply as a sequence of
//   <byte count (4 bytes, big endian)>
//   <certificate DER>
//
// To run this program you will need to do a setup like this:
//
//   export GOPATH=<enter path here...>
//   mkdir $GOPATH
//   go get github.com/agl/certificatetransparency
//   mkdir $GOPATH/mycttools
//   ln -s $(realpath dump-all-ct-certs-to-file.go) $GOPATH/mycttools
//   go run $GOPATH/mycttools/dump-all-ct-certs-to-file.go

package main

import (
  "crypto/sha1"
  "encoding/binary"
	"fmt"
	ct "github.com/agl/certificatetransparency"
	"os"
  "path"
	"sync"
  "time"
)

type EntryValue struct {
  cert []byte
  extraCertsIds []uint16
}

type ExtraCertsValue struct {
  extraCerts [][]byte
}

type State struct {
  // This lock must be held when modifying values in State.
  lock *sync.Mutex

  // A map from the hash of a certificate to its position (starting from
  // 0). There are a small number of extra certificates (intermediates) on the
  // order of thousands. It is much more efficient to just store them once,
  // since they are duplicated many times in the paths, and can be compressed
  // to a 16-bit integer.
  extraCertHashToIdMap map[string]uint16

  // The number of entries (certificate paths) visited.
  numEntries int64

  startTime time.Time

  extraCertsWriterChannel chan ExtraCertsValue
  entryWriterChannel chan EntryValue
}

func createFile(path string) *os.File {
  file, err := os.Create(path)
  if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to open output file %s: %s\n", path, err)
		os.Exit(1)
  }
  return file
}

func initState(state *State) {
	state.lock = new(sync.Mutex)
  state.extraCertHashToIdMap = make(map[string]uint16)
  state.numEntries = 0
  state.startTime = time.Now()
  state.extraCertsWriterChannel = make(chan ExtraCertsValue, 100)
  state.entryWriterChannel = make(chan EntryValue, 100)
}

func closeState(state *State) {
  close(state.extraCertsWriterChannel)
  close(state.entryWriterChannel)
}

func calculateHashes(dataArray [][]byte) []string {
  hashes := make([]string, len(dataArray))
  for i, data := range dataArray {
    hash := sha1.Sum(data)
    hashes[i] = string(hash[:])
  }
  return hashes
}

func addEntry(state *State, entry *ct.Entry) {
  if entry.Type != ct.X509Entry {
    return
  }

  // The array of extra certificates.
  extraCerts := entry.ExtraCerts

  // Calculate hashes for each extra cert (outside the lock)
  extraCertsHashes := calculateHashes(extraCerts)

  extraCertsIds := make([]uint16, len(extraCerts))
  newExtraCerts := make([][]byte, 0, len(extraCerts))

  // --------------------
  // WITHIN LOCK
  // --------------------
  state.lock.Lock()

  state.numEntries += 1

  // Fill extraCertsIds[] with the ID for each extraCert. If it is a
  // newly seen extra cert, then add it to newExtraCerts.
  for i, key := range extraCertsHashes {
    id, found := state.extraCertHashToIdMap[key]
    if !found {
      id = uint16(len(state.extraCertHashToIdMap))
      state.extraCertHashToIdMap[key] = id
      newExtraCerts = append(newExtraCerts, extraCerts[i])
    }
    extraCertsIds[i] = id
  }

  state.extraCertsWriterChannel <- ExtraCertsValue{newExtraCerts}
  state.entryWriterChannel <- EntryValue{entry.X509Cert, extraCertsIds}

  numEntriesProcessed := state.numEntries

  // --------------------
  state.lock.Unlock()
  // --------------------

  // Print status update.
  if numEntriesProcessed % 50000 == 0 {
    elapsedMinutes := float64(time.Since(state.startTime)) / float64(time.Minute)
    fraction := float64(numEntriesProcessed) / float64(10100000)
    remainingMinutes := (float64(1) - fraction) * (elapsedMinutes / fraction)
    fmt.Fprintf(os.Stderr, "[%.0f%%] Processed %d entries in %0.1f minutes (%0.1f minutes remaining)\n", fraction * float64(100), numEntriesProcessed, elapsedMinutes,  remainingMinutes)
  }
}

func appendCertToFile(certBytes []byte, outFile *os.File) {
  // Write the certificate to the output file.
  byteLen := uint32(len(certBytes))

  err := binary.Write(outFile, binary.BigEndian, byteLen)
  if err != nil {
    fmt.Fprintf(os.Stderr, "binary.Write failed: %s\n", err)
    os.Exit(1)
  }

  if _, err := outFile.Write(certBytes); err != nil {
    fmt.Fprintf(os.Stderr, "Failed to write to output file: %s\n", err)
    os.Exit(1)
  }
}

func entriesFileWriterWorker(filepath string, channel chan EntryValue) {
  file := createFile(filepath)
  defer file.Close()

  for value := range channel {
    // Write the leaf cert.
    appendCertToFile(value.cert, file)

    // Write the number of extra certs.
    err := binary.Write(file, binary.BigEndian, uint16(len(value.extraCertsIds)))
    if err != nil {
      fmt.Fprintf(os.Stderr, "binary.Write failed: %s\n", err)
      os.Exit(1)
    }

    // Write the ids of the extra certs.
    err = binary.Write(file, binary.BigEndian, value.extraCertsIds)
    if err != nil {
      fmt.Fprintf(os.Stderr, "binary.Write failed: %s\n", err)
      os.Exit(1)
    }
  }
}

func extraCertsFileWriterWorker(filepath string, channel chan ExtraCertsValue) {
  file := createFile(filepath)
  defer file.Close()

  for value := range channel {
    for _, cert := range value.extraCerts {
      appendCertToFile(cert, file)
    }
  }
}

func main() {
	if len(os.Args) != 3 {
		fmt.Fprintf(os.Stderr, "Usage: %s <input CT-DB file> <output dir>\n", os.Args[0])
		os.Exit(1)
	}
	inputDbFileName := os.Args[1]
	outputPath := os.Args[2]

  // Open the input database.
	in, err := os.Open(inputDbFileName)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to open entries file: %s\n", err)
		os.Exit(1)
	}
	defer in.Close()

	inEntriesFile := ct.EntriesFile{in}

  // Create the output directory and files.
  err = os.Mkdir(outputPath, 0755)
  if err != nil {
    fmt.Fprintf(os.Stderr, "Couldn't create directory %s: %s\n", outputPath, err)
		os.Exit(1)
  }

  // Create state shared by entries workers.
  var state State
  initState(&state)
  defer closeState(&state)

  // Create workers responsible for writing the files.
  go entriesFileWriterWorker(path.Join(outputPath, "entries.bin"), state.entryWriterChannel)
  go extraCertsFileWriterWorker(path.Join(outputPath, "extra_certs.bin"), state.extraCertsWriterChannel)

  // Create a worker that writes the extra certs

  fmt.Fprintf(os.Stderr, "Dumping entries... (Time estimate is based on assumption there are 10 million entries in the CT database).\n")

	inEntriesFile.Map(func(ent *ct.EntryAndPosition, err error) {
		if err != nil {
			return
		}
    addEntry(&state, ent.Entry)
	})
}
