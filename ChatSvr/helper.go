package main

import (
    "strconv"
)
func AtoInt64FromHash (hash map[string]string ,key string) (int64) {
    v, _ := hash[key]
    n, _ := strconv.ParseInt(v, 10, 64)
    return int64(n)
}
func AtoInt32FromHash (hash map[string]string ,key string) (int32) {
    v, _ := hash[key]
    n, _ := strconv.Atoi(v)
    return int32(n)
}
