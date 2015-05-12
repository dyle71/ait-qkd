/*
 * qkd-confirmation.cpp
 * 
 * This is the implementation of the QKD postprocessing
 * confirmationentication facilities
 * 
 * Autor: Oliver Maurhart, <oliver.maurhart@ait.ac.at>
 *
 * Copyright (C) 2012-2015 AIT Austrian Institute of Technology
 * AIT Austrian Institute of Technology GmbH
 * Donau-City-Strasse 1 | 1220 Vienna | Austria
 * http://www.ait.ac.at
 *
 * This file is part of the AIT QKD Software Suite.
 *
 * The AIT QKD Software Suite is free software: you can redistribute 
 * it and/or modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation, either version 3 of 
 * the License, or (at your option) any later version.
 * 
 * The AIT QKD Software Suite is distributed in the hope that it will 
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with the AIT QKD Software Suite. 
 * If not, see <http://www.gnu.org/licenses/>.
 */


// ------------------------------------------------------------
// incs

#include <atomic>

// ait
#include <qkd/utility/bigint.h>
#include <qkd/utility/syslog.h>

#include "qkd-confirmation.h"
#include "qkd_confirmation_dbus.h"


// ------------------------------------------------------------
// defs

#define MODULE_DESCRIPTION      "This is the qkd-confirmation QKD Module."
#define MODULE_ORGANISATION     "(C)opyright 2012, 2013 AIT Austrian Institute of Technology, http://www.ait.ac.at"


// ------------------------------------------------------------
// decl


/**
 * the qkd-confirmation pimpl
 */
class qkd_confirmation::qkd_confirmation_data {
    
public:

    
    /**
     * ctor
     */
    qkd_confirmation_data() : nBadKeys(0), nConfirmedKeys(0) {};
    
    std::recursive_mutex cPropertyMutex;    /**< property mutex */

    std::atomic<uint64_t> nBadKeys;         /**< number of bad keys so far */
    std::atomic<uint64_t> nConfirmedKeys;   /**< number of confirmed keys so far */
    uint64_t nRounds;                       /**< number of confirmation rounds */
    
};


// ------------------------------------------------------------
// code


/**
 * ctor
 */
qkd_confirmation::qkd_confirmation() : qkd::module::module("confirmation", qkd::module::module_type::TYPE_CONFIRMATION, MODULE_DESCRIPTION, MODULE_ORGANISATION) {

    d = boost::shared_ptr<qkd_confirmation::qkd_confirmation_data>(new qkd_confirmation::qkd_confirmation_data());
    
    // apply default values
    set_rounds(10);
    
    // enforce DBus registration
    new ConfirmationAdaptor(this);
}


/**
 * apply the loaded key value map to the module
 * 
 * @param   sURL            URL of config file loaded
 * @param   cConfig         map of key --> value
 */
void qkd_confirmation::apply_config(UNUSED std::string const & sURL, qkd::utility::properties const & cConfig) {
    
    // delve into the given config
    for (auto const & cEntry : cConfig) {
        
        // grab any key which is intended for us
        if (!is_config_key(cEntry.first)) continue;
        
        // ignore standard config keys: they should have been applied already
        if (is_standard_config_key(cEntry.first)) continue;
        
        std::string sKey = cEntry.first.substr(config_prefix().size());

        // module specific config here
        if (sKey == "rounds") {
            set_rounds(atoll(cEntry.second.c_str()));
        }
        else {
            qkd::utility::syslog::warning() << __FILENAME__ << '@' << __LINE__ << ": " << "found unknown key: \"" << cEntry.first << "\" - don't know how to handle this.";
        }
    }
}


/**
 * get the number of bad keys so far
 * 
 * bad keys are keys not been successfully corrected
 * 
 * @return  the number of bad keys
 */
qulonglong qkd_confirmation::bad_keys() const {
    return d->nBadKeys;
}


/**
 * get the number of confirmed keys so far
 * 
 * @return  the number of confirmation keys done
 */
qulonglong qkd_confirmation::confirmed_keys() const {
    return d->nConfirmedKeys;
}


/**
 * module work
 * 
 * @param   cKey                    the key to confirm
 * @param   cIncomingContext        incoming crypto context
 * @param   cOutgoingContext        outgoing crypto context
 * @return  always true
 */
bool qkd_confirmation::process(qkd::key::key & cKey, qkd::crypto::crypto_context & cIncomingContext, qkd::crypto::crypto_context & cOutgoingContext) {

    if (is_alice()) return process_alice(cKey, cIncomingContext, cOutgoingContext);
    if (is_bob()) return process_bob(cKey, cIncomingContext, cOutgoingContext);
    
    // should not happen to reach this line, but 
    // we return true: pass on the key to the next module
    return true;
}


/**
 * module work as alice
 * 
 * @param   cKey                    the key to confirm
 * @param   cIncomingContext        incoming crypto context
 * @param   cOutgoingContext        outgoing crypto context
 * @return  always true
 */
bool qkd_confirmation::process_alice(qkd::key::key & cKey, qkd::crypto::crypto_context & cIncomingContext, qkd::crypto::crypto_context & cOutgoingContext) {
    
    qkd::module::message cMessage;
    uint64_t nRounds = rounds();
    
    qkd::utility::bigint cKeyBI = qkd::utility::bigint(cKey.data());

    // first some header to ensure we are talking about the same key and setting
    cMessage.data() << cKey.id();
    cMessage.data() << cKey.size();
    cMessage.data() << nRounds;

    // create the masks
    std::list<bool> cParities;
    for (uint64_t i = 0; i < nRounds; i++) {
        
        // create a random bigint
        qkd::utility::memory cMemory(cKey.data().size());
        random() >> cMemory;
        qkd::utility::bigint cBI(cMemory);
        
        cBI &= cKeyBI;
        cParities.push_back(cBI.parity());
        
        // record random memory in message
        cMessage.data() << cMemory;
    }
    
    // finalize parities in message to send
    for (auto bParity : cParities) cMessage.data() << bParity;
    
    // send to bob: no timeout
    try {
        send(cMessage, cOutgoingContext, -1);
    }
    catch (std::runtime_error const & cRuntimeError) {
        qkd::utility::syslog::crit() << __FILENAME__ << '@' << __LINE__ << ": " << "failed to send message: " << cRuntimeError.what();
        return false;
    }
    
    // recv from bob's parities: no timeout
    try {
        if (!recv(cMessage, cIncomingContext, qkd::module::message_type::MESSAGE_TYPE_DATA, -1)) return false;
    }
    catch (std::runtime_error const & cRuntimeError) {
        qkd::utility::syslog::crit() << __FILENAME__ << '@' << __LINE__ << ": " << "failed to receive message: " << cRuntimeError.what();
        return false;
    }

    // compare parities
    bool bParitiesEqual = true;
    std::list<bool>::const_iterator iter = cParities.begin();
    for (uint64_t i = 0; i < nRounds; i++) {
        
        bool bParitiesBob;
        cMessage.data() >> bParitiesBob;
        bParitiesEqual = bParitiesEqual && ((*iter) == bParitiesBob);
        ++iter;
    }
    
    // match?
    if (!bParitiesEqual) {
        d->nBadKeys++;
        qkd::utility::syslog::info() << "confirmation for key " << cKey.id() << " failed";
    }
    else {
        cKey.meta().eKeyState = qkd::key::key_state::KEY_STATE_CONFIRMED;
        d->nConfirmedKeys++;
        qkd::utility::debug() << "confirmation for key " << cKey.id() << " ok";
    }

    return bParitiesEqual;
}


/**
 * module work as bob
 * 
 * @param   cKey                    the key to confirm
 * @param   cIncomingContext        incoming crypto context
 * @param   cOutgoingContext        outgoing crypto context
 * @return  always true
 */
bool qkd_confirmation::process_bob(qkd::key::key & cKey, qkd::crypto::crypto_context & cIncomingContext, qkd::crypto::crypto_context & cOutgoingContext) {
    
    qkd::module::message cMessage;
    
    qkd::key::key_id nPeerKeyId = 0;
    uint64_t nPeerKeySize = 0;
    uint64_t nRounds = 0;
    
    // recv data from alice: no timeout
    try {
        if (!recv(cMessage, cIncomingContext, qkd::module::message_type::MESSAGE_TYPE_DATA, -1)) return false;
    }
    catch (std::runtime_error const & cRuntimeError) {
        qkd::utility::syslog::crit() << __FILENAME__ << '@' << __LINE__ << ": " << "failed to receive message: " << cRuntimeError.what();
        return false;
    }
    cMessage.data() >> nPeerKeyId;
    cMessage.data() >> nPeerKeySize;
    cMessage.data() >> nRounds;
    
    set_rounds(nRounds);
    
    // sanity check
    if ((cKey.id() != nPeerKeyId) || (cKey.data().size() != nPeerKeySize)) {
        qkd::utility::syslog::warning() << __FILENAME__ << '@' << __LINE__ << ": " << "alice has wrong key id and/or different key size";
        return false;
    }
    
    // work on the received data
    std::list<bool> cParities;
    qkd::utility::bigint cKeyBI = qkd::utility::bigint(cKey.data());
    for (uint64_t i = 0; i < nRounds; i++) {
        
        qkd::utility::memory cMemory;
        cMessage.data() >> cMemory;
        
        qkd::utility::bigint cBI(cMemory);
        cBI &= cKeyBI;
        cParities.push_back(cBI.parity());
    }
    
    // compare local and peer parities
    bool bParitiesEqual = true;
    std::list<bool>::const_iterator iter = cParities.begin();
    for (uint64_t i = 0; i < nRounds; i++) {
        
        bool bParitiesAlice;
        cMessage.data() >> bParitiesAlice;
        bParitiesEqual = bParitiesEqual && ((*iter) == bParitiesAlice);
        ++iter;
    }
    
    // send our parities back to alice: no timeout
    cMessage = qkd::module::message();
    for (auto bParity : cParities) cMessage.data() << bParity;
    
    try {
        send(cMessage, cOutgoingContext, -1);
    }
    catch (std::runtime_error const & cRuntimeError) {
        qkd::utility::syslog::crit() << __FILENAME__ << '@' << __LINE__ << ": " << "failed to send message: " << cRuntimeError.what();
        return false;
    }
    
    // match?
    if (!bParitiesEqual) {
        d->nBadKeys++;
        qkd::utility::syslog::info() << "confirmation for key " << cKey.id() << " failed";
    }
    else {
        cKey.meta().eKeyState = qkd::key::key_state::KEY_STATE_CONFIRMED;
        d->nConfirmedKeys++;
        qkd::utility::debug() << "confirmation for key " << cKey.id() << " ok";
    }
    
    return bParitiesEqual;
}


/**
 * get the number of confirmation rounds
 * 
 * @return  the number of confirmation rounds done
 */
qulonglong qkd_confirmation::rounds() const {
    
    // get exclusive access to properties
    std::lock_guard<std::recursive_mutex> cLock(d->cPropertyMutex);
    return d->nRounds;
}


/**
 * set the new number of confirmation rounds
 * 
 * @param   nRounds     the new number of confirmation rounds
 */
void qkd_confirmation::set_rounds(qulonglong nRounds) {
    
    // get exclusive access to properties
    std::lock_guard<std::recursive_mutex> cLock(d->cPropertyMutex);
    d->nRounds = nRounds;
}
