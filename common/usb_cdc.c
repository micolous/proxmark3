/*
 * at91sam7s USB CDC device implementation
 *
 * Copyright (c) 2012, Roel Verdult
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the
 * names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * based on the "Basic USB Example" from ATMEL (doc6123.pdf)
 *
 * @file usb_cdc.c
 * @brief
 */

#include "usb_cdc.h"
#include "at91sam7s512.h"
#include "config_gpio.h"

#define MS_OS_20_SET_HEADER_DESCRIPTOR 0x00
#define MS_OS_20_SUBSET_HEADER_CONFIGURATION 0x01
#define MS_OS_20_SUBSET_HEADER_FUNCTION 0x02
#define MS_OS_20_FEATURE_COMPATIBLE_ID 0x03
#define MS_OS_20_FEATURE_REG_PROPERTY 0x04
#define MS_OS_20_DESCRIPTOR_LENGTH 0xd8

#define AT91C_EP_CONTROL     0
#define AT91C_EP_IN_SIZE  0x40
#define AT91C_EP_OUT         1
#define AT91C_EP_OUT_SIZE 0x40
#define AT91C_EP_IN          2

#define STR_LANGUAGE_CODES 0x00
#define STR_MANUFACTURER   0x01
#define STR_PRODUCT        0x02
#define STR_INTASSOC_1     0x03
#define STR_INTASSOC_2     0x04


static const char devDescriptor[] = {
	/* Device descriptor */
	0x12,      // bLength
	0x01,      // bDescriptorType
	0x10,0x02, // Complies with USB Spec. Release (0210h = release 2.1)
	           // USB 2.1 is needed to trigger Windows to do USB BOS.
	0xef,      // bDeviceClass:    Miscellaneous
	0x02,      // bDeviceSubclass: 
	0x01,      // bDeviceProtocol: Interface Association Descriptor
	0x08,      // bMaxPacketSize0
	0xc4,0x9a, // Vendor ID (0x9ac4 = J. Westhues)
	0x8f,0x4b, // Product ID (0x4b8f = Proxmark-3 RFID Instrument)
	0x01,0x00, // Device release number (0001)
	0x01,      // iManufacturer
	0x02,      // iProduct
	0x00,      // iSerialNumber
	0x01       // bNumConfigs
};

static const char cfgDescriptor[] = {
	/* ============== CONFIGURATION 1 =========== */
	/* Configuration 1 descriptor */
	0x09,   // CbLength
	0x02,   // CbDescriptorType
	0x5a,   // CwTotalLength 2 EP + Control (was 0x43)
	0x00,
	0x03,   // CbNumInterfaces
	0x01,   // CbConfigurationValue
	0x00,   // CiConfiguration
	0xB0,   // CbmAttributes 0xA0
	0x4B,   // CMaxPower

	/* Communication Class Interface Descriptor Requirement (0x00) */
	0x09, // bLength
	0x04, // bDescriptorType
	0x00, // bInterfaceNumber
	0x00, // bAlternateSetting
	0x01, // bNumEndpoints
	0x02, // bInterfaceClass
	0x02, // bInterfaceSubclass
	0x01, // bInterfaceProtocol
	0x00, // iInterface

	/* Header Functional Descriptor */
	0x05, // bFunction Length
	0x24, // bDescriptor type: CS_INTERFACE
	0x00, // bDescriptor subtype: Header Func Desc
	0x10, // bcdCDC:1.1
	0x01,

	/* ACM Functional Descriptor */
	0x04, // bFunctionLength
	0x24, // bDescriptor Type: CS_INTERFACE
	0x02, // bDescriptor Subtype: ACM Func Desc
	0x02, // bmCapabilities

	/* Union Functional Descriptor */
	0x05, // bFunctionLength
	0x24, // bDescriptorType: CS_INTERFACE
	0x06, // bDescriptor Subtype: Union Func Desc
	0x00, // bMasterInterface: Communication Class Interface
	0x01, // bSlaveInterface0: Data Class Interface

	/* Call Management Functional Descriptor */
	0x05, // bFunctionLength
	0x24, // bDescriptor Type: CS_INTERFACE
	0x01, // bDescriptor Subtype: Call Management Func Desc
	0x00, // bmCapabilities: D1 + D0
	0x01, // bDataInterface: Data Class Interface 1

	/* Endpoint 1 descriptor */
	0x07,   // bLength
	0x05,   // bDescriptorType
	0x83,   // bEndpointAddress, Endpoint 03 - IN
	0x03,   // bmAttributes      INT
	0x08,   // wMaxPacketSize
	0x00,
	0xFF,   // bInterval

	/* Data Class Interface Descriptor Requirement (0x01) */
	0x09, // bLength
	0x04, // bDescriptorType
	0x01, // bInterfaceNumber
	0x00, // bAlternateSetting
	0x02, // bNumEndpoints
	0x0A, // bInterfaceClass
	0x00, // bInterfaceSubclass
	0x00, // bInterfaceProtocol
	0x00, // iInterface

	/* First alternate setting */
	/* Endpoint 1 descriptor */
	0x07,   // bLength
	0x05,   // bDescriptorType
	0x01,   // bEndpointAddress, Endpoint 01 - OUT
	0x02,   // bmAttributes      BULK
	0x00,   // wMaxPacketSize "shall be set to 1024 bytes for BULK"
	0x04,
	0x00,   // bInterval

	/* Endpoint 2 descriptor */
	0x07,   // bLength
	0x05,   // bDescriptorType
	0x82,   // bEndpointAddress, Endpoint 02 - IN
	0x02,   // bmAttributes      BULK
	0x00,   // wMaxPacketSize "shall be set to 1024 bytes for BULK"
	0x04,
	0x00,   // bInterval
	
	// WebUSB interface (0x02)
	0x09, // bLength
	0x04, // bDescriptorType
	0x02, // bInterfaceNumber
	0x00, // bAlternateSetting
	0x02, // bNumEndpoints
	0xff, // bInterfaceClass
	0x00, // bInterfaceSubclass
	0x00, // bInterfaceProtocol
	0x00, // iInterface

	// Endpoint 1 descriptor
	0x07,   // bLength
	0x05,   // bDescriptorType
	0x01,   // bEndpointAddress, Endpoint 01 - OUT
	0x02,   // bmAttributes      BULK
	0x00,   // wMaxPacketSize "shall be set to 1024 bytes for BULK"
	0x04,
	0x00,   // bInterval

	// Endpoint 2 descriptor
	0x07,   // bLength
	0x05,   // bDescriptorType
	0x82,   // bEndpointAddress, Endpoint 02 - IN
	0x02,   // bmAttributes      BULK
	0x00,   // wMaxPacketSize "shall be set to 1024 bytes for BULK"
	0x04,
	0x00    // bInterval
};

static const char StrDescLanguageCodes[] = {
  4,			// Length
  0x03,			// Type is string
  0x09, 0x04	// supported language Code 0 = 0x0409 (English)
};
	
static const char StrDescManufacturer[] = {
  26,			// Length
  0x03,			// Type is string
  'p', 0x00,
  'r', 0x00,
  'o', 0x00,
  'x', 0x00,
  'm', 0x00,
  'a', 0x00,
  'r', 0x00,
  'k', 0x00,
  '.', 0x00,
  'o', 0x00,
  'r', 0x00,
  'g', 0x00
};

static const char StrDescProduct[] = {
  8,			// Length
  0x03,			// Type is string
  'P', 0x00,
  'M', 0x00,
  '3', 0x00
};


const char* getStringDescriptor(uint8_t idx)
{
	switch (idx) {
		case STR_LANGUAGE_CODES:
			return StrDescLanguageCodes;
		case STR_MANUFACTURER:
			return StrDescManufacturer;
		case STR_PRODUCT:
			return StrDescProduct;
		default:
			return NULL;
	}
}

// WebUSB / WINUSB
// Adapted from:
// https://github.com/webusb/arduino/blob/gh-pages/library/WebUSB/WebUSB.cpp

static const char bosDescriptor[] = {
0x05,  // Length
0x0F,  // Binary Object Store descriptor
0x39, 0x00,  // Total length
0x02,  // Number of device capabilities

// WebUSB Platform Capability descriptor (bVendorCode == 0x01).
0x18,  // Length
0x10,  // Device Capability descriptor
0x05,  // Platform Capability descriptor
0x00,  // Reserved
0x38, 0xB6, 0x08, 0x34, 0xA9, 0x09, 0xA0, 0x47,
0x8B, 0xFD, 0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,  // WebUSB GUID
0x00, 0x01,  // Version 1.0
0x01,  // Vendor request code
0x01,  // Landing page

// Microsoft OS 2.0 Platform Capability Descriptor (MS_VendorCode == 0x02)
0x1C,  // Length
0x10,  // Device Capability descriptor
0x05,  // Platform Capability descriptor
0x00,  // Reserved
0xDF, 0x60, 0xDD, 0xD8, 0x89, 0x45, 0xC7, 0x4C,
0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,  // MS OS 2.0 GUID
0x00, 0x00, 0x03, 0x06,  // Windows version
MS_OS_20_DESCRIPTOR_LENGTH, 0x00,  // Descriptor set length
0x02,  // Vendor request code
0x00   // Alternate enumeration code
};

static const char msos2Descriptor[] = {
// Spec: http://download.microsoft.com/download/3/5/6/3563ED4A-F318-4B66-A181-AB1D8F6FD42D/MS_OS_2_0_desc.docx
// Microsoft OS 2.0 descriptor set header (table 10)
// This is needed to make Windows handle composite devices properly.
// Useful info: http://goo.gl/ZDmsnt

0x0A, 0x00,  // Descriptor size (10 bytes)
MS_OS_20_SET_HEADER_DESCRIPTOR, 0x00,  // MS OS 2.0 descriptor set header
0x00, 0x00, 0x03, 0x06,  // Windows version (8.1) (0x06030000)
MS_OS_20_DESCRIPTOR_LENGTH, 0x00,  // Size, MS OS 2.0 descriptor set

// Microsoft OS 2.0 registry property descriptor (Table 14)
// This registry key is created at:
// HKLM\SYSTEM\CurrentControlSet\Enum\USB\VID_9AC4&PID_4B8F\...\Device Parameters
0x26, 0x00,   // wLength
MS_OS_20_FEATURE_REG_PROPERTY, 0x00,
0x01, 0x00,   // wPropertyDataType: REG_SZ
0x14, 0x00,   // wPropertyNameLength

'p', 0x00, 'r', 0x00, 'o', 0x00, 'x', 0x00, 'm', 0x00, 'a', 0x00, 'r', 0x00,
'k', 0x00, '3', 0x00, 0x00, 0x00,

0x08, 0x00,   // wPropertyDataLength
'P', 0x00, 'M', 0x00, '3', 0x00, 0x00, 0x00,


// Microsoft OS 2.0 configuration subset header
0x08, 0x00,  // Descriptor size (8 bytes)
 MS_OS_20_SUBSET_HEADER_CONFIGURATION, 0x00,  // MS OS 2.0 configuration subset header
0x00,        // bConfigurationValue
0x00,        // Reserved
0xA8, 0x00,  // Size, MS OS 2.0 configuration subset

// Microsoft OS 2.0 function subset header
0x08, 0x00,  // Descriptor size (8 bytes)
 MS_OS_20_SUBSET_HEADER_FUNCTION, 0x00,  // MS OS 2.0 function subset header
0x00, // First interface number for WebUSB (1 byte) sent here.
0x00,        // Reserved
0xA0, 0x00,  // Size, MS OS 2.0 function subset


// Microsoft OS 2.0 compatible ID descriptor (table 13)
0x14, 0x00,  // wLength
MS_OS_20_FEATURE_COMPATIBLE_ID, 0x00,  // MS_OS_20_FEATURE_COMPATIBLE_ID
'W',  'I',  'N',  'U',  'S',  'B',  0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,


// Custom Property
// This is needed so that Windows will properly enumerate the composite device.
0x84, 0x00,   //wLength: 
MS_OS_20_FEATURE_REG_PROPERTY, 0x00,   // wDescriptorType: MS_OS_20_FEATURE_REG_PROPERTY: 0x04 (Table 9)
0x07, 0x00,   //wPropertyDataType: REG_MULTI_SZ (Table 15)
0x2a, 0x00,   //wPropertyNameLength: 
//bPropertyName: “DeviceInterfaceGUID”
'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00, 
'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00, 
0x00, 0x00,
0x50, 0x00,   // wPropertyDataLength
//bPropertyData: "{43222fae-0732-4f6c-acb6-cb91f222a77c}"
'{', 0x00, '4', 0x00, '3', 0x00, '2', 0x00, '2', 0x00, '2', 0x00, 'F', 0x00, 'A', 0x00, 'E', 0x00, '-', 0x00,
'0', 0x00, '7', 0x00, '3', 0x00, '2', 0x00, '-', 0x00, '4', 0x00, 'F', 0x00, '6', 0x00, 'C', 0x00, '-', 0x00,
'A', 0x00, 'C', 0x00, 'B', 0x00, '6', 0x00, '-', 0x00, 'C', 0x00, 'B', 0x00, '9', 0x00, '1', 0x00, 'F', 0x00,
'2', 0x00, '2', 0x00, '2', 0x00, 'A', 0x00, '7', 0x00, '7', 0x00, 'C', 0x00, '}', 0x00,
0x00, 0x00, 0x00, 0x00
};

static const char demoWebUsbUrl[] = {
	3 + 28, // length
	0x03, // WEBUSB_URL
	0x00, // Scheme (0x00: http, 0x01: https)
	0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x3a, 0x38, 0x30,
	0x30, 0x30, 0x2f, 0x70, 0x72, 0x6f, 0x78, 0x6d, 0x61, 0x72, 0x6b, 0x2e,
	0x68, 0x74, 0x6d, 0x6c	// "localhost:8000/proxmark.html"
};

const char* getWebUsbUrl(uint8_t idx)
{
	switch (idx) {
		case 0x01:
			return demoWebUsbUrl;
		default:
			return NULL;
	}
}

// Bitmap for all status bits in CSR which must be written as 1 to cause no effect
#define REG_NO_EFFECT_1_ALL      AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1 \
                                |AT91C_UDP_STALLSENT   | AT91C_UDP_RXSETUP \
                                |AT91C_UDP_TXCOMP

// Clear flags in the UDP_CSR register
#define UDP_CLEAR_EP_FLAGS(endpoint, flags) { \
	volatile unsigned int reg; \
	reg = pUdp->UDP_CSR[(endpoint)]; \
	reg |= REG_NO_EFFECT_1_ALL; \
	reg &= ~(flags); \
	pUdp->UDP_CSR[(endpoint)] = reg; \
} 

// Set flags in the UDP_CSR register
#define UDP_SET_EP_FLAGS(endpoint, flags) { \
	volatile unsigned int reg; \
	reg = pUdp->UDP_CSR[(endpoint)]; \
	reg |= REG_NO_EFFECT_1_ALL; \
	reg |= (flags); \
	pUdp->UDP_CSR[(endpoint)] = reg; \
}

/* USB standard request codes */
#define STD_GET_STATUS_ZERO           0x0080
#define STD_GET_STATUS_INTERFACE      0x0081
#define STD_GET_STATUS_ENDPOINT       0x0082

#define STD_CLEAR_FEATURE_ZERO        0x0100
#define STD_CLEAR_FEATURE_INTERFACE   0x0101
#define STD_CLEAR_FEATURE_ENDPOINT    0x0102

#define STD_SET_FEATURE_ZERO          0x0300
#define STD_SET_FEATURE_INTERFACE     0x0301
#define STD_SET_FEATURE_ENDPOINT      0x0302

#define STD_SET_ADDRESS               0x0500
#define STD_GET_DESCRIPTOR            0x0680
#define STD_SET_DESCRIPTOR            0x0700
#define STD_GET_CONFIGURATION         0x0880
#define STD_SET_CONFIGURATION         0x0900
#define STD_GET_INTERFACE             0x0A81
#define STD_SET_INTERFACE             0x0B01
#define STD_SYNCH_FRAME               0x0C82

/* CDC Class Specific Request Code */
#define GET_LINE_CODING               0x21A1
#define SET_LINE_CODING               0x2021
#define SET_CONTROL_LINE_STATE        0x2221

/* Vendor-specific control requests */
#define VEN_CTRL_WEBUSB               0x01C0
#define WEBUSB_REQUEST_GET_URL        0x02
#define VEN_CTRL_MS_OS_20             0x02C0
#define MS_OS_20_REQUEST_DESCRIPTOR   0x07


typedef struct {
	unsigned int dwDTERRate;
	char bCharFormat;
	char bParityType;
	char bDataBits;
} AT91S_CDC_LINE_CODING, *AT91PS_CDC_LINE_CODING;

AT91S_CDC_LINE_CODING line = {
	115200, // baudrate
	0,      // 1 Stop Bit
	0,      // None Parity
	8};     // 8 Data bits

void AT91F_CDC_Enumerate();

AT91PS_UDP pUdp = AT91C_BASE_UDP;
byte_t btConfiguration = 0;
byte_t btConnection    = 0;
byte_t btReceiveBank   = AT91C_UDP_RX_DATA_BK0;

//*----------------------------------------------------------------------------
//* \fn    usb_disable
//* \brief This function deactivates the USB device
//*----------------------------------------------------------------------------
void usb_disable() {
  // Disconnect the USB device
  AT91C_BASE_PIOA->PIO_ODR = GPIO_USB_PU;
  
  // Clear all lingering interrupts
  if(pUdp->UDP_ISR & AT91C_UDP_ENDBUSRES) {
    pUdp->UDP_ICR = AT91C_UDP_ENDBUSRES;
  }
}

//*----------------------------------------------------------------------------
//* \fn    usb_enable
//* \brief This function Activates the USB device
//*----------------------------------------------------------------------------
void usb_enable() {
  // Set the PLL USB Divider
  AT91C_BASE_CKGR->CKGR_PLLR |= AT91C_CKGR_USBDIV_1 ;
  
  // Specific Chip USB Initialisation
  // Enables the 48MHz USB clock UDPCK and System Peripheral USB Clock
  AT91C_BASE_PMC->PMC_SCER = AT91C_PMC_UDP;
  AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_UDP);
  
  // Enable UDP PullUp (USB_DP_PUP) : enable & Clear of the corresponding PIO
  // Set in PIO mode and Configure in Output
  AT91C_BASE_PIOA->PIO_PER = GPIO_USB_PU; // Set in PIO mode
	AT91C_BASE_PIOA->PIO_OER = GPIO_USB_PU; // Configure as Output
  
  // Clear for set the Pullup resistor
	AT91C_BASE_PIOA->PIO_CODR = GPIO_USB_PU;
  
  // Disconnect and reconnect USB controller for 100ms
  usb_disable();
  
  // Wait for a short while
  for (volatile size_t i=0; i<0x100000; i++);

  // Reconnect USB reconnect
  AT91C_BASE_PIOA->PIO_SODR = GPIO_USB_PU;
  AT91C_BASE_PIOA->PIO_OER = GPIO_USB_PU;
}

//*----------------------------------------------------------------------------
//* \fn    usb_check
//* \brief Test if the device is configured and handle enumeration
//*----------------------------------------------------------------------------
bool usb_check() {
	AT91_REG isr = pUdp->UDP_ISR;

	if (isr & AT91C_UDP_ENDBUSRES) {
		pUdp->UDP_ICR = AT91C_UDP_ENDBUSRES;
		// reset all endpoints
		pUdp->UDP_RSTEP  = (unsigned int)-1;
		pUdp->UDP_RSTEP  = 0;
		// Enable the function
		pUdp->UDP_FADDR = AT91C_UDP_FEN;
		// Configure endpoint 0
		pUdp->UDP_CSR[AT91C_EP_CONTROL] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_CTRL);
	}
	else if (isr & AT91C_UDP_EPINT0) {
		pUdp->UDP_ICR = AT91C_UDP_EPINT0;
		AT91F_CDC_Enumerate();
	}
	return (btConfiguration) ? true : false;
}


bool usb_poll()
{
  if (!usb_check()) return false;
  return (pUdp->UDP_CSR[AT91C_EP_OUT] & btReceiveBank);
}

/**
	In github PR #129, some users appears to get a false positive from
	usb_poll, which returns true, but the usb_read operation
	still returns 0.
	This check is basically the same as above, but also checks
	that the length available to read is non-zero, thus hopefully fixes the
	bug.
**/
bool usb_poll_validate_length()
{

	if (!usb_check()) return false;
	if (!(pUdp->UDP_CSR[AT91C_EP_OUT] & btReceiveBank)) return false;
	return (pUdp->UDP_CSR[AT91C_EP_OUT] >> 16) >  0;
}

//*----------------------------------------------------------------------------
//* \fn    usb_read
//* \brief Read available data from Endpoint OUT
//*----------------------------------------------------------------------------
uint32_t usb_read(byte_t* data, size_t len) {
	byte_t bank = btReceiveBank;
	uint32_t packetSize, nbBytesRcv = 0;
	uint32_t time_out = 0;
  
	while (len)  {
		if (!usb_check()) break;

		if ( pUdp->UDP_CSR[AT91C_EP_OUT] & bank ) {
			packetSize = MIN(pUdp->UDP_CSR[AT91C_EP_OUT] >> 16, len);
			len -= packetSize;
			while(packetSize--)
				data[nbBytesRcv++] = pUdp->UDP_FDR[AT91C_EP_OUT];
			UDP_CLEAR_EP_FLAGS(AT91C_EP_OUT, bank);
			if (bank == AT91C_UDP_RX_DATA_BK0) {
				bank = AT91C_UDP_RX_DATA_BK1;
			} else {
				bank = AT91C_UDP_RX_DATA_BK0;
			}
		}
		if (time_out++ == 0x1fff) break;
	}

	btReceiveBank = bank;
	return nbBytesRcv;
}

//*----------------------------------------------------------------------------
//* \fn    usb_write
//* \brief Send through endpoint 2
//*----------------------------------------------------------------------------
uint32_t usb_write(const byte_t* data, const size_t len) {
  size_t length = len;
	uint32_t cpt = 0;

  if (!length) return 0;
  if (!usb_check()) return 0;
  
	// Send the first packet
	cpt = MIN(length, AT91C_EP_IN_SIZE-1);
	length -= cpt;
	while (cpt--) pUdp->UDP_FDR[AT91C_EP_IN] = *data++;
	UDP_SET_EP_FLAGS(AT91C_EP_IN, AT91C_UDP_TXPKTRDY);

	while (length) {
		// Fill the second bank
		cpt = MIN(length, AT91C_EP_IN_SIZE-1);
		length -= cpt;
		while (cpt--) pUdp->UDP_FDR[AT91C_EP_IN] = *data++;
		// Wait for the first bank to be sent
		while (!(pUdp->UDP_CSR[AT91C_EP_IN] & AT91C_UDP_TXCOMP)) {
			if (!usb_check()) return length;
    }
		UDP_CLEAR_EP_FLAGS(AT91C_EP_IN, AT91C_UDP_TXCOMP);
		while (pUdp->UDP_CSR[AT91C_EP_IN] & AT91C_UDP_TXCOMP);
		UDP_SET_EP_FLAGS(AT91C_EP_IN, AT91C_UDP_TXPKTRDY);
	}
  
	// Wait for the end of transfer
	while (!(pUdp->UDP_CSR[AT91C_EP_IN] & AT91C_UDP_TXCOMP)) {
		if (!usb_check()) return length;
  }
  
	UDP_CLEAR_EP_FLAGS(AT91C_EP_IN, AT91C_UDP_TXCOMP);
	while (pUdp->UDP_CSR[AT91C_EP_IN] & AT91C_UDP_TXCOMP);

	return length;
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_USB_SendData
//* \brief Send Data through the control endpoint
//*----------------------------------------------------------------------------
unsigned int csrTab[100] = {0x00};
unsigned char csrIdx = 0;

static void AT91F_USB_SendData(AT91PS_UDP pUdp, const char *pData, uint32_t length) {
	uint32_t cpt = 0;
	AT91_REG csr;

	do {
		cpt = MIN(length, 8);
		length -= cpt;

		while (cpt--)
			pUdp->UDP_FDR[0] = *pData++;

		if (pUdp->UDP_CSR[AT91C_EP_CONTROL] & AT91C_UDP_TXCOMP) {
			UDP_CLEAR_EP_FLAGS(AT91C_EP_CONTROL, AT91C_UDP_TXCOMP);
			while (pUdp->UDP_CSR[AT91C_EP_CONTROL] & AT91C_UDP_TXCOMP);
		}

		UDP_SET_EP_FLAGS(AT91C_EP_CONTROL, AT91C_UDP_TXPKTRDY);
		do {
			csr = pUdp->UDP_CSR[AT91C_EP_CONTROL];

			// Data IN stage has been stopped by a status OUT
			if (csr & AT91C_UDP_RX_DATA_BK0) {
				UDP_CLEAR_EP_FLAGS(AT91C_EP_CONTROL, AT91C_UDP_RX_DATA_BK0);
				return;
			}
		} while ( !(csr & AT91C_UDP_TXCOMP) );

	} while (length);

	if (pUdp->UDP_CSR[AT91C_EP_CONTROL] & AT91C_UDP_TXCOMP) {
		UDP_CLEAR_EP_FLAGS(AT91C_EP_CONTROL, AT91C_UDP_TXCOMP);
		while (pUdp->UDP_CSR[AT91C_EP_CONTROL] & AT91C_UDP_TXCOMP);
	}
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_USB_SendZlp
//* \brief Send zero length packet through the control endpoint
//*----------------------------------------------------------------------------
void AT91F_USB_SendZlp(AT91PS_UDP pUdp) {
	UDP_SET_EP_FLAGS(AT91C_EP_CONTROL, AT91C_UDP_TXPKTRDY);
	while ( !(pUdp->UDP_CSR[AT91C_EP_CONTROL] & AT91C_UDP_TXCOMP) );
	UDP_CLEAR_EP_FLAGS(AT91C_EP_CONTROL, AT91C_UDP_TXCOMP);
	while (pUdp->UDP_CSR[AT91C_EP_CONTROL] & AT91C_UDP_TXCOMP);
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_USB_SendStall
//* \brief Stall the control endpoint
//*----------------------------------------------------------------------------
void AT91F_USB_SendStall(AT91PS_UDP pUdp) {
	UDP_SET_EP_FLAGS(AT91C_EP_CONTROL, AT91C_UDP_FORCESTALL);
	while ( !(pUdp->UDP_CSR[AT91C_EP_CONTROL] & AT91C_UDP_ISOERROR) );
	UDP_CLEAR_EP_FLAGS(AT91C_EP_CONTROL, AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR);
	while (pUdp->UDP_CSR[AT91C_EP_CONTROL] & (AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR));
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_CDC_Enumerate
//* \brief This function is a callback invoked when a SETUP packet is received
//*----------------------------------------------------------------------------
void AT91F_CDC_Enumerate() {
	byte_t bmRequestType, bRequest, bHighValue, bLowValue;
	uint16_t wValue, wIndex, wLength, wStatus;

	if ( !(pUdp->UDP_CSR[AT91C_EP_CONTROL] & AT91C_UDP_RXSETUP) )
		return;

	bmRequestType = pUdp->UDP_FDR[0];
	bRequest      = pUdp->UDP_FDR[0];

	bLowValue     = pUdp->UDP_FDR[0];
	bHighValue    = pUdp->UDP_FDR[0];
	wValue        = (bLowValue & 0xFF);
	wValue       |= (bHighValue << 8);
	
	wIndex        = (pUdp->UDP_FDR[0] & 0xFF);
	wIndex       |= (pUdp->UDP_FDR[0] << 8);
	wLength       = (pUdp->UDP_FDR[0] & 0xFF);
	wLength      |= (pUdp->UDP_FDR[0] << 8);

	if (bmRequestType & 0x80) {
		UDP_SET_EP_FLAGS(AT91C_EP_CONTROL, AT91C_UDP_DIR);
		while ( !(pUdp->UDP_CSR[AT91C_EP_CONTROL] & AT91C_UDP_DIR) );
	}
	UDP_CLEAR_EP_FLAGS(AT91C_EP_CONTROL, AT91C_UDP_RXSETUP);
	while ( (pUdp->UDP_CSR[AT91C_EP_CONTROL] & AT91C_UDP_RXSETUP)  );

	// Handle supported standard device request Cf Table 9-3 in USB specification Rev 1.1
	switch ((bRequest << 8) | bmRequestType) {
	case STD_GET_DESCRIPTOR:
		switch (bHighValue) {
		case 0x01: // Return Device Descriptor
			AT91F_USB_SendData(pUdp, devDescriptor, MIN(sizeof(devDescriptor), wLength));
			break;
		case 0x02: // Return Configuration Descriptor
			AT91F_USB_SendData(pUdp, cfgDescriptor, MIN(sizeof(cfgDescriptor), wLength));
			break;
		case 0x03: // Return String Descriptor
		{
			const char *strDescriptor = getStringDescriptor(bLowValue);
			if (strDescriptor != NULL) {
				AT91F_USB_SendData(pUdp, strDescriptor, MIN(strDescriptor[0], wLength));
			} else {
				AT91F_USB_SendStall(pUdp);
			}
			break;
		}
		case 0x0F: // Return Binary Object Store (BOS)
		{
			if (bLowValue == 0 && wIndex == 0) {
				AT91F_USB_SendData(pUdp, bosDescriptor, MIN(sizeof(bosDescriptor), wLength));
			} else {
				AT91F_USB_SendStall(pUdp);
			}
				
			break;
		}
		default:
			AT91F_USB_SendStall(pUdp);
			break;
		}
		break;
	case STD_SET_ADDRESS:
		AT91F_USB_SendZlp(pUdp);
		pUdp->UDP_FADDR = (AT91C_UDP_FEN | wValue);
		pUdp->UDP_GLBSTATE  = (wValue) ? AT91C_UDP_FADDEN : 0;
		break;
	case STD_SET_CONFIGURATION:
		btConfiguration = wValue;
		AT91F_USB_SendZlp(pUdp);
		pUdp->UDP_GLBSTATE  = (wValue) ? AT91C_UDP_CONFG : AT91C_UDP_FADDEN;
		pUdp->UDP_CSR[1] = (wValue) ? (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT) : 0;
		pUdp->UDP_CSR[2] = (wValue) ? (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN)  : 0;
		pUdp->UDP_CSR[3] = (wValue) ? (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_INT_IN)   : 0;
		break;
	case STD_GET_CONFIGURATION:
		AT91F_USB_SendData(pUdp, (char *) &(btConfiguration), sizeof(btConfiguration));
		break;
	case STD_GET_STATUS_ZERO:
		wStatus = 0;
		AT91F_USB_SendData(pUdp, (char *) &wStatus, sizeof(wStatus));
		break;
	case STD_GET_STATUS_INTERFACE:
		wStatus = 0;
		AT91F_USB_SendData(pUdp, (char *) &wStatus, sizeof(wStatus));
		break;
	case STD_GET_STATUS_ENDPOINT:
		wStatus = 0;
		wIndex &= 0x0F;
		if ((pUdp->UDP_GLBSTATE & AT91C_UDP_CONFG) && (wIndex <= 3)) {
			wStatus = (pUdp->UDP_CSR[wIndex] & AT91C_UDP_EPEDS) ? 0 : 1;
			AT91F_USB_SendData(pUdp, (char *) &wStatus, sizeof(wStatus));
		}
		else if ((pUdp->UDP_GLBSTATE & AT91C_UDP_FADDEN) && (wIndex == 0)) {
			wStatus = (pUdp->UDP_CSR[wIndex] & AT91C_UDP_EPEDS) ? 0 : 1;
			AT91F_USB_SendData(pUdp, (char *) &wStatus, sizeof(wStatus));
		}
		else
			AT91F_USB_SendStall(pUdp);
		break;
	case STD_SET_FEATURE_ZERO:
		AT91F_USB_SendStall(pUdp);
	    break;
	case STD_SET_FEATURE_INTERFACE:
		AT91F_USB_SendZlp(pUdp);
		break;
	case STD_SET_FEATURE_ENDPOINT:
		wIndex &= 0x0F;
		if ((wValue == 0) && wIndex && (wIndex <= 3)) {
			pUdp->UDP_CSR[wIndex] = 0;
			AT91F_USB_SendZlp(pUdp);
		}
		else
			AT91F_USB_SendStall(pUdp);
		break;
	case STD_CLEAR_FEATURE_ZERO:
		AT91F_USB_SendStall(pUdp);
	    break;
	case STD_CLEAR_FEATURE_INTERFACE:
		AT91F_USB_SendZlp(pUdp);
		break;
	case STD_CLEAR_FEATURE_ENDPOINT:
		wIndex &= 0x0F;
		if ((wValue == 0) && wIndex && (wIndex <= 3)) {
			if (wIndex == 1)
				pUdp->UDP_CSR[1] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT);
			else if (wIndex == 2)
				pUdp->UDP_CSR[2] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN);
			else if (wIndex == 3)
				pUdp->UDP_CSR[3] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_ISO_IN);
			AT91F_USB_SendZlp(pUdp);
		}
		else
			AT91F_USB_SendStall(pUdp);
		break;

	// handle CDC class requests
	case SET_LINE_CODING:
		while ( !(pUdp->UDP_CSR[AT91C_EP_CONTROL] & AT91C_UDP_RX_DATA_BK0) );
		UDP_CLEAR_EP_FLAGS(AT91C_EP_CONTROL, AT91C_UDP_RX_DATA_BK0);
		AT91F_USB_SendZlp(pUdp);
		break;
	case GET_LINE_CODING:
		AT91F_USB_SendData(pUdp, (char *) &line, MIN(sizeof(line), wLength));
		break;
	case SET_CONTROL_LINE_STATE:
		btConnection = wValue;
		AT91F_USB_SendZlp(pUdp);
		break;
	
	// Handle vendor-specific requests
	case VEN_CTRL_WEBUSB:
		if (wIndex == WEBUSB_REQUEST_GET_URL) {
			const char *url = getWebUsbUrl(bLowValue);
			if (url != NULL) {
				AT91F_USB_SendData(pUdp, url, MIN(url[0], wLength));
			} else {
				AT91F_USB_SendStall(pUdp);
			}
		} else {
			AT91F_USB_SendStall(pUdp);
		}		
		break;
	case VEN_CTRL_MS_OS_20:
		if (wIndex == MS_OS_20_REQUEST_DESCRIPTOR) {
			AT91F_USB_SendData(pUdp, msos2Descriptor, MIN(sizeof(msos2Descriptor), wLength));
		} else {
			AT91F_USB_SendStall(pUdp);
		}			
		break;
	default:
		AT91F_USB_SendStall(pUdp);
	    break;
	}
}
