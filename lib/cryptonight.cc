// Copyright (c) 2017 ExcitableAardvark <excitableaardvark@tutanota.de>
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <nan.h>
#include <node_buffer.h>

#include "vendor/hash.h"

using namespace crypto;
using namespace Nan;
using namespace node;
using namespace v8;

NAN_METHOD(Hash) {
	bool fast = false;
	if (info.Length() < 1)
		return Nan::ThrowError("At least one argument is required");
	if (info.Length() > 1) {
		if(!info[1]->IsBoolean())
			return Nan::ThrowError("Argument 2 should be a boolean");
		fast = info[1]->ToBoolean()->BooleanValue();
	}

	Local<Object> buf = info[0]->ToObject();

	if (!Buffer::HasInstance(buf))
		return Nan::ThrowError("Argument must be a buffer");

	hash x;

	if (fast) {
		cn_fast_hash(Buffer::Data(buf), Buffer::Length(buf), x);
	} else {
		cn_slow_hash(Buffer::Data(buf), Buffer::Length(buf), x);
	}

	info.GetReturnValue().Set(CopyBuffer(x.data, 32).ToLocalChecked());
}

class CnWorker : public AsyncWorker {
public:
	CnWorker(Callback *callback, char *data, int length, bool fast)
		 : AsyncWorker(callback), data(data), length(length), fast(fast) {}
	~CnWorker() {}

	void Execute() {
		if (fast) {
			cn_fast_hash(data, length, hresult);
		} else {
			cn_slow_hash(data, length, hresult);
		}
	}

	void HandleOKCallback() {
		Local<Value> argv[] = {
			CopyBuffer(hresult.data, 32).ToLocalChecked()
		};

		callback->Call(1, argv);
	}

private:
	char *data;
	int length;
	hash hresult;
	bool fast;
};

NAN_METHOD(AsyncHash) {
	bool fast = false;
	int callbackIndex = 1;
	if (info.Length() < 2)
		return Nan::ThrowError("At least two arguments are required");

	Local<Object> buf = info[0]->ToObject();

	if (!Buffer::HasInstance(buf))
		return Nan::ThrowError("First argument must be a buffer");

	if (info.Length() > 2) {
		if(!info[1]->IsBoolean())
			return Nan::ThrowError("Argument 2 should be a boolean");
		fast = info[1]->ToBoolean()->BooleanValue();
		callbackIndex = 2;
	}


	char *data = Buffer::Data(buf);
	int len = Buffer::Length(buf);

	Callback *callback = new Callback(To<Function>(info[callbackIndex]).ToLocalChecked());

	AsyncQueueWorker(new CnWorker(callback, data, len, fast));
}

NAN_MODULE_INIT(CnInit) {
	Nan::Set(target, New<String>("hash").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(Hash)).ToLocalChecked());
	Nan::Set(target, New<String>("asyncHash").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(AsyncHash)).ToLocalChecked());
}

NODE_MODULE(cryptonight, CnInit)

