#!/usr/local/bin/thrift --gen cpp

namespace cpp CS6210

struct URLMap
{
	1: map<string,string> URLBody
}

service FetchURL {
	string fetch(1:string URL)
}
