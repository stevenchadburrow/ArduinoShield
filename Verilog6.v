`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    05:32:52 07/17/2023 
// Design Name: 
// Module Name:    Verilog6 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////

// This is experimenting with doubling the RAM to 16KB.
module Verilog6(
	input master_clock, // 25.175 MHz
	output video_latch, // latches video shift register
	output video_visible,
	output write_enable, // controls RAM /WE signal
	output [15:0] address, // connects to RAM and ROM addresses
	output high_addr,
	output [7:0] data, // connects to RAM and ROM
	output reg hsync, // connected to VGA
	output reg vsync, // connected to VGA
	input serial_clock, // clock signal for input shift register
	input serial_data, // data from the input shift register
	output serial_latch // latches data from RAM into output shift register
);

reg half_clock; // 12.58 MHz
reg quarter_clock; // 6.29 MHz
reg eighth_clock; // 3.14 MHz
reg [16:0] video_addr;
reg hblank;
reg vblank;
reg [23:0] shift_value;
reg shift_ready;
reg shift_clear; 


//assign high_addr = (eighth_clock && shift_ready) ? shift_value[21] : 1'b0;
assign high_addr = 1'bz;

assign address[5:0] = (eighth_clock && shift_ready) ? shift_value[13:8] : video_addr[5:0];
assign address[8:6] = (eighth_clock && shift_ready) ? 3'b000 : video_addr[10:8];
assign address[15:9] = (eighth_clock && shift_ready) ? shift_value[20:14] : video_addr[16:11];
assign data[7:0] = (eighth_clock && shift_ready && shift_value[22]) ? shift_value[7:0] : 8'bzzzzzzzz;
assign write_enable = (/*~half_clock && */quarter_clock && eighth_clock && shift_ready && shift_value[22]) ? 1'b0 : 1'b1;
assign video_latch = (half_clock && quarter_clock && ~eighth_clock) ? 1'b0 : 1'b1;
assign video_visible = (hblank && vblank) ? 1'b1 : 1'b0;
assign serial_latch = (/*~half_clock && */quarter_clock && eighth_clock && shift_ready && ~shift_value[22]) ? 1'b0 : 1'b1;


always @(posedge master_clock) begin
	if (half_clock) begin
		if (quarter_clock) begin
			if (eighth_clock) begin

				if (shift_ready) begin
					shift_ready <= 1'b0;
					shift_clear <= 1'b1;
				end
					
				if (shift_value[23]) begin
					if (~shift_clear) begin
						shift_ready <= 1'b1;
					end
				end
				else begin
					shift_clear <= 1'b0;
				end
				
				if (video_addr[6:0] == 7'b1100010) begin
					if (video_addr[7]) begin
						if (video_addr[16:8] == 9'b100000110) begin
							video_addr[16:8] <= 9'b000000000;
						end
						else begin
							video_addr[16:8] <= video_addr[16:8] + 1;
						end
					end
						
					video_addr[7] <= ~video_addr[7];
					
					video_addr[6:0] <= 7'b000000;						
				end
				else begin
					video_addr[6:0] <= video_addr[6:0] + 1;
				end
			end
			else begin
				if (video_addr[6:0] == 7'b1000000) begin
					hblank <= 1'b0;
				end
				
				if (video_addr[6:0] == 7'b1001010) begin // 7'b1010000
					hsync <= 1'b0;
				end
					
				if (video_addr[6:0] == 7'b1010110) begin // 7'b1011100
					hsync <= 1'b1;
				end
				
				if (video_addr[6:0] == 7'b0000000) begin
					hblank <= 1'b1;
				end
				
				if (video_addr[6:0] == 7'b1100010) begin
					if (video_addr[7]) begin
						if (video_addr[16:8] == 9'b011101111) begin
							vblank <= 1'b0;
						end
						
						if (video_addr[16:8] == 9'b011110101) begin
							vsync <= 1'b0;
						end
						
						if (video_addr[16:8] == 9'b011110110) begin
							vsync <= 1'b1;
						end
						
						if (video_addr[16:8] == 9'b100000110) begin
							vblank <= 1'b1;
						end
					end
				end
			end
			
			eighth_clock <= ~eighth_clock;
		end
		
		quarter_clock <= ~quarter_clock;
	end
	
	half_clock <= ~half_clock;

end

always @(posedge serial_clock) begin
	
	if (shift_value[23] && shift_clear) begin
		shift_value[23:0] <= 24'b000000000000000000000000;
	end
	else begin
		shift_value[23:1] <= shift_value[22:0];
		
		shift_value[0] <= serial_data;
	end
end

endmodule



