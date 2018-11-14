//
//  PeerManager.c
//
//  Created by Mihail Gutan on 12/11/15.
//  Copyright (c) 2015 bitwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include "PeerManager.h"
#include "BitWPeer.h"
#include "BitWBIP39Mnemonic.h"
#include "BitWInt.h"
#include "BitWMerkleBlock.h"
#include "BitWallet.h"
#include "wallet.h"
#include <pthread.h>
#include <BitWBase58.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#define fprintf(...) __android_log_print(ANDROID_LOG_ERROR, "btn", _va_rest(__VA_ARGS__, NULL))

