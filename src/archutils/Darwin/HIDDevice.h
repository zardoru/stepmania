#ifndef HIDDEVICE_H
#define HIDDEVICE_H

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/usb/USB.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <vector>
#include <utility>
#include <unordered_map>

#include "RageLog.h"
#include "RageInputDevice.h"

using std::vector;

/* A few helper functions. */

// The result needs to be released.
inline CFNumberRef CFInt( int n )
{
	return CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &n );
}

inline void PrintIOErr( IOReturn err, const char *s )
{
	LOG->Warn( "%s - %s(%x,%d)", s, mach_error_string(err), err, err & 0xFFFFFF );
}

inline Boolean IntValue( CFTypeRef o, int &n )
{
	if( !o || CFGetTypeID(o) != CFNumberGetTypeID() )
		return false;
	return CFNumberGetValue( CFNumberRef(o), kCFNumberIntType, &n );
}

inline Boolean LongValue( CFTypeRef o, long &n )
{
	if( !o || CFGetTypeID(o) != CFNumberGetTypeID() )
		return false;
	return CFNumberGetValue( CFNumberRef(o), kCFNumberLongType, &n );
}

/* This is just awful, these aren't objects, treating them as such
 * leads to: (*object)->function(object [, argument]...)
 * Instead, do: CALL(object, function [, argument]...)
 */
#define CALL(o,f,...) (*(o))->f((o), ## __VA_ARGS__)

class HIDDevice
{
private:
	IOHIDDeviceInterface **m_Interface;
	IOHIDQueueInterface **m_Queue;
	bool m_bRunning;
	RString m_sDescription;

	static void AddLogicalDevice( const void *value, void *context );
	static void AddElement( const void *value, void *context );

protected:
	/* Each physical device has zero or more logical devices. If this device
	 * allows a logical device of type (usagePage, usage), then allocate storage
	 * as necessary and return true, otherwise, return false. */
	virtual bool AddLogicalDevice( int usagePage, int usage ) = 0;

	/* If the most recently added logical device cares about the state of an
	 * element of type (usagePage, usage), store the cookie. */
	virtual void AddElement( int usagePage, int usage, IOHIDElementCookie cookie, const CFDictionaryRef properties ) = 0;

	// Add any elements to the queue by calling AddElementToQueue() with the stored cookies.
	virtual void Open() = 0;

	// Optional. Subclasses can initialize the device, if required.
	virtual bool InitDevice( int vid, int pid ) { return true; }

	// This adds the element with the given cookie to the queue to be notified of state changes.
	inline void AddElementToQueue( IOHIDElementCookie cookie )
	{
		IOReturn ret = CALL( m_Queue, addElement, cookie, 0 );
		if( ret != KERN_SUCCESS )
			LOG->Warn( "Failed to add HID element with cookie %p to queue: %u", cookie, ret );
	}

	// Perform a synchronous set report on the HID interface.
	inline IOReturn SetReport( IOHIDReportType type, UInt32 reportID, void *buffer, UInt32 size, UInt32 timeoutMS )
	{
		return CALL( m_Interface, setReport, type, reportID, buffer, size, timeoutMS, nullptr, nullptr, nullptr );
	}
public:
	HIDDevice();
	virtual ~HIDDevice();

	bool Open( io_object_t device );
	void StartQueue( CFRunLoopRef loopRef, IOHIDCallbackFunction callback, void *target, int refCon );
	inline const RString& GetDescription() const { return m_sDescription; }

	/* Add button presses (or releases) to vPresses for the given cookie. More
	 * than one DeviceInput can be added at a time. For example, Two axes
	 * presses may be generated by a single element. The value of the element is
	 * passed to determine if this is a push or a release. The time is provided
	 * as an optimization. */
	virtual void GetButtonPresses( vector<DeviceInput>& vPresses, IOHIDElementCookie cookie, int value, const RageTimer& now ) const = 0;

	/* Returns the number of IDs assigned starting from startID. This is not
	 * meaningful for devices like keyboards that all share the same InputDevice
	 * id. If a particular device has multiple logical devices, then it must
	 * ensure that AssignIDs does not assign an ID outside of its range.
	 * Return -1 to indicate that the device does not share the same InputDevice
	 * and none could be assigned. */
	virtual int AssignIDs( InputDevice startID ) { return 0; }

	//Add a device and a description for each logical device.
	virtual void GetDevicesAndDescriptions( vector<InputDeviceInfo>& vDevices ) const = 0;
};

#endif

/*
 * (c) 2005-2006 Steve Checkoway
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
