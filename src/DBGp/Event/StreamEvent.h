// {{{ Copyright notice
/* Copyright (c) 2007, Adam Harvey
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *  - The names of its contributors may not be used to endorse or promote
 *  products derived from this software without specific prior written
 *  permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
// }}}

#ifndef DBGP_EVENT_STREAMEVENT_H
#define DBGP_EVENT_STREAMEVENT_H

#include <wx/event.h>
#include <wx/string.h>

#include "DBGp/Event/Event.h"

/** The event type for stream events. */
extern const wxEventType wxEVT_DBGP_STREAM;

namespace DBGp {
	/** Base class for stream-related events. */
	class StreamEvent : public Event {
		public:
			/**
			 * Constructs a new stream event.
			 *
			 * @param[in] conn The DBGp connection.
			 * @param[in] data The data sent to the stream.
			 * @param[in] type The event type.
			 */
			StreamEvent(Connection *conn, const wxString &data, wxEventType type);

			/**
			 * Copy constructor for stream events.
			 *
			 * @param[in] event
			 */
			StreamEvent(const StreamEvent &event);

			/**
			 * Returns the data sent to the stream.
			 *
			 * @return The data.
			 */
			inline wxString GetData() const { return data; }

			/**
			 * Returns a new copy of the event.
			 *
			 * @return A new copy of the event.
			 */
			inline wxEvent *Clone() const { return new StreamEvent(*this); }

		protected:
			/** The data sent to the stream. */
			wxString data;
	};
}

/** Typedef for stream event handlers. */
typedef void (wxEvtHandler::*DBGpStreamFunction)(DBGp::StreamEvent &);

/** Event table macro. */
#define EVT_DBGP_STREAM(id, fn) \
	DECLARE_EVENT_TABLE_ENTRY(wxEVT_DBGP_STREAM, id, -1, \
		(wxObjectEventFunction) (wxEventFunction) (DBGpFunction) \
		wxStaticCastEvent(DBGpStreamFunction, &fn), (wxObject *) NULL),

#endif

// vim:set fdm=marker ts=8 noet cin: