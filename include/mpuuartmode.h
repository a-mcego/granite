#pragma once

#include <queue>
#include "interrupt.h"

// MPU-401 UART Mode implementation
// Hardcoded to I/O 330h, IRQ 9 (typical defaults)
// Only implements UART mode - intelligent mode not supported

struct MPUUartMode
{
    CHIP8259& pic;

    MPUUartMode(CHIP8259& pic_) : pic(pic_)
    {
    }

    enum PORTS
    {
        DATA = 0x00,        // 330h - bidirectional data port
        STATUS_COMMAND = 0x01  // 331h - status when read, command when written
    };

    enum COMMANDS
    {
        RESET = 0xFF,       // Reset MPU to intelligent mode
        UART_MODE = 0x3F    // Switch to UART mode (no ACK)
    };

    enum STATUS_BITS
    {
        DSR = 0x80,         // Data Set Ready - clear when data available to read
        DRR = 0x40          // Data Read Ready - clear when OK to write
    };

    bool uart_mode{false};
    bool reset_pending{false};
    std::queue<u8> input_buffer;   // MIDI bytes received from external source
    std::queue<u8> output_buffer;  // MIDI bytes to send to external destination

    u8 status_register{0x00};     // Bit 7=DSR, Bit 6=DRR

    u8 read(u8 port)
    {
        u8 data = 0;

        if (port == DATA)
        {
            // Read from data port - get MIDI byte or ACK
            if (!input_buffer.empty())
            {
                data = input_buffer.front();
                input_buffer.pop();

                // Update DSR bit - clear if more data available
                if (input_buffer.empty())
                {
                    status_register &= ~DSR;  // Clear DSR - no more data
                }
            }
        }
        else if (port == STATUS_COMMAND)
        {
            // Read status port
            data = status_register;

            // DSR bit: clear (0) when data available to read
            if (!input_buffer.empty())
                data |= DSR;
            else
                data &= ~DSR;

            // DRR bit: clear (0) when OK to write
            // Set (1) when MPU is busy or has unread data
            if (input_buffer.empty())
                data &= ~DRR;  // OK to write
            else
                data |= DRR;   // Not OK to write - must read data first
        }

        return data;
    }

    void write(u8 port, u8 data)
    {
        if (port == DATA)
        {
            // Write MIDI byte to data port
            if (uart_mode)
            {
                // In UART mode - just output the MIDI byte
                output_buffer.push(data);
                // In a real implementation, this would go to MIDI OUT
                // For now, we'll just store it in the output buffer
            }
        }
        else if (port == STATUS_COMMAND)
        {
            // Write command to command port
            if (data == RESET)
            {
                // Reset command - clear buffers, go to intelligent mode
                while (!input_buffer.empty()) input_buffer.pop();
                while (!output_buffer.empty()) output_buffer.pop();
                uart_mode = false;
                status_register = 0x00;

                // Send ACK (0xFE) for reset command
                input_buffer.push(0xFE);
                status_register |= DSR;  // Data available to read
            }
            else if (data == UART_MODE)
            {
                // Switch to UART mode - no ACK sent for this command
                uart_mode = true;
                status_register = 0x00;
                // Clear any pending data
                while (!input_buffer.empty()) input_buffer.pop();
            }
            // In UART mode, all other commands are ignored
            // In intelligent mode, we don't implement other commands
        }
    }

    // Call this to simulate receiving a MIDI byte from external source
    void receive_midi_byte(u8 midi_byte)
    {
        if (uart_mode)
        {
            input_buffer.push(midi_byte);
            status_register |= DSR;  // Signal data available

            // Trigger interrupt when MIDI data arrives
            pic.request_interrupt(9);  // IRQ 9 for MPU-401
        }
    }

    // Call this to get the next MIDI byte to send to external destination
    // Returns true if byte was available, false if output buffer empty
    bool get_output_midi_byte(u8& midi_byte)
    {
        if (!output_buffer.empty())
        {
            midi_byte = output_buffer.front();
            output_buffer.pop();
            return true;
        }
        return false;
    }

    // Helper functions for checking status (matching the assembly examples)
    bool is_input_available() const
    {
        return !input_buffer.empty();
    }

    bool is_output_ready() const
    {
        // OK to write when no unread input data is pending
        return input_buffer.empty();
    }

    void cycle()
    {
        // MPU-401 doesn't need regular cycling like sound cards
        // MIDI I/O is event-driven, not sample-based
        // This could be used for timing if implementing intelligent mode features
    }
};
