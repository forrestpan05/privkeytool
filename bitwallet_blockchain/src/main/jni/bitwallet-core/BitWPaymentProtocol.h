//
//  BitWPaymentProtocol.h
//
//  Created by Aaron Voisine on 9/7/15.
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

#ifndef BitWPaymentProtocol_h
#define BitWPaymentProtocol_h

#include "BitWTransaction.h"
#include "BitWAddress.h"
#include "BitWKey.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// BIP70 payment protocol: https://github.com/bitcoin/bips/blob/master/bip-0070.mediawiki
// BIP75 payment protocol encryption: https://github.com/bitcoin/bips/blob/master/bip-0075.mediawiki

typedef struct {
    char *network; // "main" or "test", default is "main"
    BitWTxOutput *outputs; // where to send payments, outputs[n].amount defaults to 0
    size_t outCount;
    uint64_t time; // request creation time, seconds since unix epoch, optional
    uint64_t expires; // when this request should be considered invalid, optional
    char *memo; // human-readable description of request for the customer, optional
    char *paymentURL; // url to send payment and get payment ack, optional
    uint8_t *merchantData; // arbitrary data to include in the payment message, optional
    size_t merchDataLen;
} BitWPaymentProtocolDetails;

// returns a newly allocated details struct that must be freed by calling BitWPaymentProtocolDetailsFree()
BitWPaymentProtocolDetails *BitWPaymentProtocolDetailsNew(const char *network, const BitWTxOutput outputs[], size_t outCount,
                                                      uint64_t time, uint64_t expires, const char *memo,
                                                      const char *paymentURL, const uint8_t *merchantData,
                                                      size_t merchDataLen);

// buf must contain a serialized details struct
// returns a details struct that must be freed by calling BitWPaymentProtocolDetailsFree()
BitWPaymentProtocolDetails *BitWPaymentProtocolDetailsParse(const uint8_t *buf, size_t bufLen);

// writes serialized details struct to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BitWPaymentProtocolDetailsSerialize(const BitWPaymentProtocolDetails *details, uint8_t *buf, size_t bufLen);

// frees memory allocated for details struct
void BitWPaymentProtocolDetailsFree(BitWPaymentProtocolDetails *details);

typedef struct {
    uint32_t version; // default is 1
    char *pkiType; // none / x509+sha256 / x509+sha1, default is "none"
    uint8_t *pkiData; // depends on pkiType, optional
    size_t pkiDataLen;
    BitWPaymentProtocolDetails *details; // required
    uint8_t *signature; // pki-dependent signature, optional
    size_t sigLen;
} BitWPaymentProtocolRequest;

// returns a newly allocated request struct that must be freed by calling BitWPaymentProtocolRequestFree()
BitWPaymentProtocolRequest *BitWPaymentProtocolRequestNew(uint32_t version, const char *pkiType, const uint8_t *pkiData,
                                                      size_t pkiDataLen, BitWPaymentProtocolDetails *details,
                                                      const uint8_t *signature, size_t sigLen);

// buf must contain a serialized request struct
// returns a request struct that must be freed by calling BitWPaymentProtocolRequestFree()
BitWPaymentProtocolRequest *BitWPaymentProtocolRequestParse(const uint8_t *buf, size_t bufLen);

// writes serialized request struct to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BitWPaymentProtocolRequestSerialize(const BitWPaymentProtocolRequest *req, uint8_t *buf, size_t bufLen);

// writes the DER encoded certificate corresponding to index to cert
// returns the number of bytes written to cert, or the total certLen needed if cert is NULL
// returns 0 if index is out-of-bounds
size_t BitWPaymentProtocolRequestCert(const BitWPaymentProtocolRequest *req, uint8_t *cert, size_t certLen, size_t idx);

// writes the hash of the request to md needed to sign or verify the request
// returns the number of bytes written, or the total mdLen needed if md is NULL
size_t BitWPaymentProtocolRequestDigest(BitWPaymentProtocolRequest *req, uint8_t *md, size_t mdLen);

// frees memory allocated for request struct
void BitWPaymentProtocolRequestFree(BitWPaymentProtocolRequest *req);

typedef struct {
    uint8_t *merchantData; // from request->details->merchantData, optional
    size_t merchDataLen;
    BitWTransaction **transactions; // array of signed BitWTransaction struct references to satisfy outputs from details
    size_t txCount;
    BitWTxOutput *refundTo; // where to send refunds, if a refund is necessary, refundTo[n].amount defaults to 0
    size_t refundToCount;
    char *memo; // human-readable message for the merchant, optional
} BitWPaymentProtocolPayment;

// returns a newly allocated payment struct that must be freed by calling BitWPaymentProtocolPaymentFree()
BitWPaymentProtocolPayment *BitWPaymentProtocolPaymentNew(const uint8_t *merchantData, size_t merchDataLen,
                                                      BitWTransaction *transactions[], size_t txCount,
                                                      const uint64_t refundToAmounts[],
                                                      const BitWAddress refundToAddresses[], size_t refundToCount,
                                                      const char *memo);

// buf must contain a serialized payment struct
// returns a payment struct that must be freed by calling BitWPaymentProtocolPaymentFree()
BitWPaymentProtocolPayment *BitWPaymentProtocolPaymentParse(const uint8_t *buf, size_t bufLen);

// writes serialized payment struct to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BitWPaymentProtocolPaymentSerialize(const BitWPaymentProtocolPayment *payment, uint8_t *buf, size_t bufLen);

// frees memory allocated for payment struct (does not call BitWTransactionFree() on transactions)
void BitWPaymentProtocolPaymentFree(BitWPaymentProtocolPayment *payment);

typedef struct {
    BitWPaymentProtocolPayment *payment; // payment message that triggered this ack, required
    char *memo; // human-readable message for customer, optional
} BitWPaymentProtocolACK;

// returns a newly allocated ACK struct that must be freed by calling BitWPaymentProtocolACKFree()
BitWPaymentProtocolACK *BitWPaymentProtocolACKNew(BitWPaymentProtocolPayment *payment, const char *memo);

// buf must contain a serialized ACK struct
// returns an ACK struct that must be freed by calling BitWPaymentProtocolACKFree()
BitWPaymentProtocolACK *BitWPaymentProtocolACKParse(const uint8_t *buf, size_t bufLen);

// writes serialized ACK struct to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BitWPaymentProtocolACKSerialize(const BitWPaymentProtocolACK *ack, uint8_t *buf, size_t bufLen);

// frees memory allocated for ACK struct
void BitWPaymentProtocolACKFree(BitWPaymentProtocolACK *ack);

typedef struct {
    BitWKey senderPubKey; // sender's public key, required
    uint64_t amount; // amount is integer-number-of-satoshis, defaults to 0
    char *pkiType; // none / x509+sha256, default is "none"
    uint8_t *pkiData; // depends on pkiType, optional
    size_t pkiDataLen;
    char *memo; // human-readable description of invoice request for the receiver, optional
    char *notifyUrl; // URL to notify on encrypted payment request ready, optional
    uint8_t *signature; // pki-dependent signature, optional
    size_t sigLen;
} BitWPaymentProtocolInvoiceRequest;

// returns a newly allocated invoice request struct that must be freed by calling BitWPaymentProtocolInvoiceRequestFree()
BitWPaymentProtocolInvoiceRequest *BitWPaymentProtocolInvoiceRequestNew(BitWKey *senderPubKey, uint64_t amount,
                                                                    const char *pkiType, uint8_t *pkiData,
                                                                    size_t pkiDataLen, const char *memo,
                                                                    const char *notifyUrl, const uint8_t *signature,
                                                                    size_t sigLen);
    
// buf must contain a serialized invoice request
// returns an invoice request struct that must be freed by calling BitWPaymentProtocolInvoiceRequestFree()
BitWPaymentProtocolInvoiceRequest *BitWPaymentProtocolInvoiceRequestParse(const uint8_t *buf, size_t bufLen);
    
// writes serialized invoice request to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BitWPaymentProtocolInvoiceRequestSerialize(BitWPaymentProtocolInvoiceRequest *req, uint8_t *buf, size_t bufLen);
    
// writes the DER encoded certificate corresponding to index to cert
// returns the number of bytes written to cert, or the total certLen needed if cert is NULL
// returns 0 if index is out-of-bounds
size_t BitWPaymentProtocolInvoiceRequestCert(const BitWPaymentProtocolInvoiceRequest *req, uint8_t *cert, size_t certLen,
                                           size_t idx);
    
// writes the hash of the request to md needed to sign or verify the request
// returns the number of bytes written, or the total mdLen needed if md is NULL
size_t BitWPaymentProtocolInvoiceRequestDigest(BitWPaymentProtocolInvoiceRequest *req, uint8_t *md, size_t mdLen);

// frees memory allocated for invoice request struct
void BitWPaymentProtocolInvoiceRequestFree(BitWPaymentProtocolInvoiceRequest *req);

typedef enum {
    BitWPaymentProtocolMessageTypeUnknown = 0,
    BitWPaymentProtocolMessageTypeInvoiceRequest = 1,
    BitWPaymentProtocolMessageTypeRequest = 2,
    BitWPaymentProtocolMessageTypePayment = 3,
    BitWPaymentProtocolMessageTypeACK = 4
} BitWPaymentProtocolMessageType;

typedef struct {
    BitWPaymentProtocolMessageType msgType; // message type of message, required
    uint8_t *message; // serialized payment protocol message, required
    size_t msgLen;
    uint64_t statusCode; // payment protocol status code, optional
    char *statusMsg; // human-readable payment protocol status message, optional
    uint8_t *identifier; // unique key to identify entire exchange, optional (should use sha256 of invoice request)
    size_t identLen;
} BitWPaymentProtocolMessage;

// returns a newly allocated message struct that must be freed by calling BitWPaymentProtocolMessageFree()
BitWPaymentProtocolMessage *BitWPaymentProtocolMessageNew(BitWPaymentProtocolMessageType msgType, const uint8_t *message,
                                                      size_t msgLen, uint64_t statusCode, const char *statusMsg,
                                                      const uint8_t *identifier, size_t identLen);
    
// buf must contain a serialized message
// returns an message struct that must be freed by calling BitWPaymentProtocolMessageFree()
BitWPaymentProtocolMessage *BitWPaymentProtocolMessageParse(const uint8_t *buf, size_t bufLen);
    
// writes serialized message struct to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BitWPaymentProtocolMessageSerialize(const BitWPaymentProtocolMessage *msg, uint8_t *buf, size_t bufLen);
    
// frees memory allocated for message struct
void BitWPaymentProtocolMessageFree(BitWPaymentProtocolMessage *msg);
    
typedef struct {
    BitWPaymentProtocolMessageType msgType; // message type of decrypted message, required
    uint8_t *message; // encrypted payment protocol message, required
    size_t msgLen;
    BitWKey receiverPubKey; // receiver's public key, required
    BitWKey senderPubKey; // sender's public key, required
    uint64_t nonce; // microseconds since epoch, required
    uint8_t *signature; // signature over the full encrypted message with sender/receiver ec key respectively, optional
    size_t sigLen;
    uint8_t *identifier; // unique key to identify entire exchange, optional (should use sha256 of invoice request)
    size_t identLen;
    uint64_t statusCode; // payment protocol status code, optional
    char *statusMsg; // human-readable payment protocol status message, optional
} BitWPaymentProtocolEncryptedMessage;

// returns a newly allocated encrypted message struct that must be freed with BitWPaymentProtocolEncryptedMessageFree()
// message is the un-encrypted serialized payment protocol message
// one of either receiverKey or senderKey must contain a private key, and the other must contain only a public key
BitWPaymentProtocolEncryptedMessage *BitWPaymentProtocolEncryptedMessageNew(BitWPaymentProtocolMessageType msgType,
                                                                        const uint8_t *message, size_t msgLen,
                                                                        BitWKey *receiverKey, BitWKey *senderKey,
                                                                        uint64_t nonce,
                                                                        const uint8_t *identifier, size_t identLen,
                                                                        uint64_t statusCode, const char *statusMsg);
    
// buf must contain a serialized encrytped message
// returns an encrypted message struct that must be freed by calling BitWPaymentProtocolEncryptedMessageFree()
BitWPaymentProtocolEncryptedMessage *BitWPaymentProtocolEncryptedMessageParse(const uint8_t *buf, size_t bufLen);
    
// writes serialized encrypted message to buf and returns number of bytes written, or total bufLen needed if buf is NULL
size_t BitWPaymentProtocolEncryptedMessageSerialize(BitWPaymentProtocolEncryptedMessage *msg, uint8_t *buf, size_t bufLen);

int BitWPaymentProtocolEncryptedMessageVerify(BitWPaymentProtocolEncryptedMessage *msg, BitWKey *pubKey);

size_t BitWPaymentProtocolEncryptedMessageDecrypt(BitWPaymentProtocolEncryptedMessage *msg, uint8_t *out, size_t outLen,
                                                BitWKey *privKey);

// frees memory allocated for encrypted message struct
void BitWPaymentProtocolEncryptedMessageFree(BitWPaymentProtocolEncryptedMessage *msg);

#ifdef __cplusplus
}
#endif

#endif // BitWPaymentProtocol_h
