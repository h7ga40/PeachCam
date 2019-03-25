/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized. This
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer*
* Copyright (C) 2015 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/

#include "SnapshotHandler.h"

int (*SnapshotHandler::callback_func_req)(const char ** pp_data);
int (*SnapshotHandler::callback_func_req2)(const char* rootPath, const char* path, const char ** pp_data);
void (*SnapshotHandler::callback_func_send_end)(const char* rootPath, const char* path, const char * p_data);
Semaphore SnapshotHandler::req_sem(1);


SnapshotHandler::SnapshotHandler(const char* rootPath, const char* path, TCPSocket* pTCPSocket) : HTTPRequestHandler(rootPath, path, pTCPSocket), m_err404(false)
{
}

SnapshotHandler::~SnapshotHandler()
{
}

void SnapshotHandler::doGet()
{
    const char * p_data = NULL;
    int size = 0;

    req_sem.wait();
    if (callback_func_req != NULL) {
        size = callback_func_req(&p_data);
    }
    if (callback_func_req2 != NULL) {
        size = callback_func_req2((rootPath()).c_str(), (path()).c_str(), &p_data);
    }

    if ((p_data == NULL) || (size <= 0)) {
        if (callback_func_send_end != NULL) {
            callback_func_send_end((rootPath()).c_str(), (path()).c_str(), p_data);
        }
        m_err404 = true;
        setErrCode(404);
        const char* msg = "File not found.";
        setContentLen(strlen(msg));
        respHeaders()["Content-Type"] = "text/html";
        respHeaders()["Connection"] = "close";
        writeData(msg,strlen(msg)); //Only send header
        printf("\r\nExit SnapshotHandler::doGet() w Error 404\r\n");
        req_sem.release();
        return;
    }
    send_data_buf = p_data;
    send_size = size;
    send_index = 0;

    //Response
    setContentLen(send_size);

    //Make sure that the browser won't cache this request
    respHeaders()["Cache-Control"] = "no-store";
    respHeaders()["Pragma"] = "no-cache";
    respHeaders()["Expires"] = "0";

    //Write data
    respHeaders()["Connection"] = "close";
    onWriteable();

    req_sem.release();
}

void SnapshotHandler::doPost()
{

}

void SnapshotHandler::doHead()
{

}

void SnapshotHandler::onReadable() //Data has been read
{

}

void SnapshotHandler::onWriteable() //Data has been written & buf is free
{
    if (m_err404) {
        //Error has been served, now exit
        close();
    } else {
        while (true) {
            int len = send_size - send_index;
            if (len > 0) {
                int writtenLen = writeData((char *)&send_data_buf[send_index], len);
                if (writtenLen < 0) { //Socket error
                    close();
                    if (callback_func_send_end != NULL) {
                        callback_func_send_end((rootPath()).c_str(), (path()).c_str(), send_data_buf);
                    }
                    return;
                } else {
                    send_index += writtenLen;
                }
            } else {
                close();
                if (callback_func_send_end != NULL) {
                    callback_func_send_end((rootPath()).c_str(), (path()).c_str(), send_data_buf);
                }
                return;
            }
        }
    }
}

void SnapshotHandler::onClose() //Connection is closing
{
    //Nothing to do
}

