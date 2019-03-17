The KERMIT protocol is a protocol that is part of the ARQ (Automatic Repeat Request) protocol class,
in which an incorrect or unverified package is automatically retransmitted. Useful data are packaged, being
surrounded by some control fields. There is no flow control during the transmission of a packet.
Each package must be confirmed.

The advantage of KERMIT versus similar is the simplicity of implementation, as well as:
• Negotiation of certain communication parameters between the transmitter and the receiver via the premiums
packages
• The ability to transfer multiple files within a session
• Sending filenames
• Possibility for packets to have variable types and lengths
Transferring a file is like any ARQ protocol. The receiver receives the package, and after checking
its sequence number versus the previous packet, calculates a local check amount for the
data. If the calculated control coincides with the one arrived, a positive ACK confirmation (character
or package); otherwise negative NAK will be issued. Finally, an EOF package is sent.
If there are any files to send, the next header is sent, and finally an EOT packet.
