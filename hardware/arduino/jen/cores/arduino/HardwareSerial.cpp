/*
  HardwareSerial.cpp - Hardware serial library for Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  Modified 23 November 2006 by David A. Mellis
  Modified 28 September 2010 by Mark Sproul
  Modified 14 August 2012 by Alarus
  Modified 3 December 2013 by Matthijs Kooijman
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"

#include "HardwareSerial.h"
#include "HardwareSerial_private.h"

#include <AppHardwareApi.h>

// this next line disables the entire HardwareSerial.cpp, 
// this is so I can support Attiny series and any other chip without a uart
#if defined(HAVE_HWSERIAL0) || defined(HAVE_HWSERIAL1) || defined(HAVE_HWSERIAL2) || defined(HAVE_HWSERIAL3)

// SerialEvent functions are weak, so when the user doesn't define them,
// the linker just sets their address to 0 (which is checked below).
// The Serialx_available is just a wrapper around Serialx.available(),
// but we can refer to it weakly so we don't pull in the entire
// HardwareSerial instance if the user doesn't also refer to it.
#if defined(HAVE_HWSERIAL0)
  void serialEvent() __attribute__((weak));
  bool Serial0_available() __attribute__((weak));
#endif

#if defined(HAVE_HWSERIAL1)
  void serialEvent1() __attribute__((weak));
  bool Serial1_available() __attribute__((weak));
#endif

#if defined(HAVE_HWSERIAL2)
  void serialEvent2() __attribute__((weak));
  bool Serial2_available() __attribute__((weak));
#endif

#if defined(HAVE_HWSERIAL3)
  void serialEvent3() __attribute__((weak));
  bool Serial3_available() __attribute__((weak));
#endif

void serialEventRun(void)
{
#if defined(HAVE_HWSERIAL0)
  if (Serial0_available && serialEvent && Serial0_available()) serialEvent();
#endif
#if defined(HAVE_HWSERIAL1)
  if (Serial1_available && serialEvent1 && Serial1_available()) serialEvent1();
#endif
#if defined(HAVE_HWSERIAL2)
  if (Serial2_available && serialEvent2 && Serial2_available()) serialEvent2();
#endif
#if defined(HAVE_HWSERIAL3)
  if (Serial3_available && serialEvent3 && Serial3_available()) serialEvent3();
#endif
}

// Actual interrupt handlers //////////////////////////////////////////////////////////////
void HardwareSerial::_tx_udr_empty_irq(void)
{
}
// Public Methods //////////////////////////////////////////////////////////////
void HardwareSerial::begin(unsigned long baud, byte config)
{
	//UART
	vAHI_UartEnable(E_AHI_UART_0);

	vAHI_UartReset(E_AHI_UART_0, TRUE, TRUE);
	vAHI_UartReset(E_AHI_UART_0, FALSE, FALSE);

	int c_baud;
	switch(baud)
	{
	case 4800:   c_baud = E_AHI_UART_RATE_4800;   break;
	case 9600:   c_baud = E_AHI_UART_RATE_9600;   break;
	case 19200:  c_baud = E_AHI_UART_RATE_19200;  break;
	case 38400:  c_baud = E_AHI_UART_RATE_38400;  break;
	case 76800:  c_baud = E_AHI_UART_RATE_76800;  break;
	case 115200: c_baud = E_AHI_UART_RATE_115200; break;
	default:     c_baud = -1;                     break;
	}

	uint32_t wordlen;
	switch(config & 0x6)
	{
	case 0x0: wordlen = E_AHI_UART_WORD_LEN_5;  break;
	case 0x2: wordlen = E_AHI_UART_WORD_LEN_6;  break;
	case 0x4: wordlen = E_AHI_UART_WORD_LEN_7;  break;
	case 0x6: wordlen = E_AHI_UART_WORD_LEN_8;  break;
	default:  wordlen = -1; break;
	}

	uint32_t stopbit =
		( (config & 0x08) ? E_AHI_UART_1_STOP_BIT : E_AHI_UART_1_STOP_BIT);
	uint32_t parity_enable =
		( (config & 0x10) ? E_AHI_UART_PARITY_ENABLE : E_AHI_UART_PARITY_DISABLE);
	uint32_t parity_type =
		( (config & 0x30) ? E_AHI_UART_EVEN_PARITY: E_AHI_UART_ODD_PARITY);

	vAHI_UartSetClockDivisor(E_AHI_UART_0, c_baud);
	vAHI_UartSetControl(E_AHI_UART_0, parity_type, parity_enable, wordlen, stopbit, false);

	setWriteCharFPtr( HardwareSerial::uart_write_char );
	setWriteStringFPtr( HardwareSerial::uart_write_string );

	DEBUG_STR("c_baud:");
	DEBUG_DEC(c_baud);
	DEBUG_STR("\r\n");
	DEBUG_STR("wordlen:");
	DEBUG_DEC(wordlen);
	DEBUG_STR("\r\n");
	DEBUG_STR("stopbit:");
	DEBUG_DEC(stopbit);
	DEBUG_STR("\r\n");
	DEBUG_STR("parity_enable:");
	DEBUG_DEC(parity_enable);
	DEBUG_STR("\r\n");
	DEBUG_STR("parity_type:");
	DEBUG_DEC(parity_type);
	DEBUG_STR("\r\n");
}

void HardwareSerial::end()
{
	vAHI_UartDisable(E_AHI_UART_0);
}

int HardwareSerial::available()
{
	return uart_available(this);
}

int HardwareSerial::uart_available(void* context)
{
	HardwareSerial* serial = reinterpret_cast<HardwareSerial*>(context);
	int uart = serial->uart;

	int r = u16AHI_UartReadRxFifoLevel(uart);
	if(serial->peek_buf != -1) return r++;
	DEBUG_STR("available:");
	DEBUG_DEC(r);
	DEBUG_STR("\r\n");

	return r;
}

int HardwareSerial::peek()
{
	return uart_peek(this);
}

int HardwareSerial::uart_peek(void* context)
{
	HardwareSerial* serial = reinterpret_cast<HardwareSerial*>(context);

	if(serial->peek_buf != -1) return serial->peek_buf;

	serial->peek_buf = uart_read(context);
	DEBUG_STR("peek:");
	DEBUG_DEC(serial->peek_buf);
	DEBUG_STR("\r\n");
	return serial->peek_buf;
}

int HardwareSerial::read()
{
	return uart_read(this);
}

int HardwareSerial::uart_read(void* context)
{
	HardwareSerial* serial = reinterpret_cast<HardwareSerial*>(context);
	int uart = serial->uart;
	DEBUG_STR("read:\r\n");
	int ret;
	if(serial->peek_buf != -1) {
		ret = serial->peek_buf;
		serial->peek_buf = -1;
		return ret;
	}

	if(!u8AHI_UartReadLineStatus(uart) & E_AHI_UART_LS_DR) {
		return -1;
	}
	DEBUG_STR("available:");
	DEBUG_DEC(u16AHI_UartReadRxFifoLevel(uart) );
	DEBUG_STR("\r\n");
	ret = u8AHI_UartReadData(uart);

	return ret;
}

int HardwareSerial::availableForWrite(void)
{
	return 0;
}

void HardwareSerial::flush()
{
	uart_flush(this);
}

void HardwareSerial::uart_flush(void* context)
{
	int uart = reinterpret_cast<HardwareSerial*>(context)->uart;
	while((u8AHI_UartReadLineStatus(uart) & E_AHI_UART_LS_THRE) == 0);
}

size_t HardwareSerial::uart_write_char(void* context, uint8_t c)
{
	int uart = reinterpret_cast<HardwareSerial*>(context)->uart;
	while((u8AHI_UartReadLineStatus(uart) & E_AHI_UART_LS_THRE) == 0);
	vAHI_UartWriteData(uart, c);
	return 1;
}

size_t HardwareSerial::uart_write_string(void* context, const uint8_t* s, size_t length)
{
	int uart = reinterpret_cast<HardwareSerial*>(context)->uart;
	for(uint32_t i=0; i<length; i++) {
		while((u8AHI_UartReadLineStatus(uart) & E_AHI_UART_LS_THRE) == 0);
		vAHI_UartWriteData(uart, s[i]);
	}
	return length;
}
#endif // whole file