﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Drawing;
using lib.TCPClient;
using System.IO;

namespace ComDebraFpga
{
	public partial class frmMain : Form
	{
		frmGraph gr = new frmGraph();
		frmGraph grCpu = new frmGraph();
		List<ComElem> lstLog = new List<ComElem>();
		List<ComCmd> lstCmd = new List<ComCmd>();

		clMain m;
		clDrawTable drawTable;


		internal void addLog(ComElem newVal)
		{
			lstLog.Add(newVal);
		}

		private void traitLastLog()
		{
			if (lstLog[0] == null)
				return;
			switch (lstLog[0].typeVal)
			{
				case TypeVal.info:
					rtbLog.AppendText(lstLog[0].val + "\r\n");
					break;
				case TypeVal.infoNoLn:
					rtbLog.AppendText(lstLog[0].val.ToString());
					break;       
				case TypeVal.vals:
					gr.addData((int[])lstLog[0].val);
					// rtbLog.AppendText(lstLog[0].val + "\r\n");
					break;
				case TypeVal.TcpEvent:
					clTCPClient.EventEventArgs e = (clTCPClient.EventEventArgs)lstLog[0].val;

					switch (e.TypeEvent)
					{
						case clTCPClient.EtatConn.TCP_ERROR:
							butConnect.Text = "Connect";
							butConnect.Enabled = true;
							rtbLog.AppendText(e.Data + "\r\n");
							break;
						case clTCPClient.EtatConn.CLIENT_CONNECTED:
							butConnect.Text = "Disconnect";
							butConnect.Enabled = true;
							break;
						case clTCPClient.EtatConn.CLIENT_DISCONNECTED:
							butConnect.Text = "Connect";
							butConnect.Enabled = true;
							break;
					}
					break;
				default:
					rtbLog.AppendText(lstLog[0].val + "\r\n");
					break;
			}

			if (rtbLog.Lines.Length > 1 && ( rtbLog.Lines.Length > 1000 || rtbLog.Lines[rtbLog.Lines.Length - 1] == "cls"))
				rtbLog.Text = "";
		}

		string lineGraph = "";
		private void checkForGraph(string p)
		{
			if (gr == null || gr.IsDisposed)
				return;

			if (p == "\r")
			{
				if (lineGraph.StartsWith("gr"))
				{
					gr.addData(lineGraph.Substring(2));
				}
				lineGraph = "";
			}
			else
			{
				lineGraph += p;
			}
		}

		private void ProcessCmd(int p)
		{
			ComCmd curCmd = null;
			for (int i = 0; i < lstCmd.Count; i++)
			{
				string a = dataButs.Rows[p].Cells[0].Value.ToString();
				if (lstCmd[i].label == a)
				{
					curCmd = lstCmd[i];
					break;
				}
			}

			string param = dataButs.Rows[p].Cells[1].Value.ToString();
			string[] paramStr = param.Split(new char[] { ',' });
			int[] paramInt = new int[paramStr.Length];
			for (int i = 0; i < paramInt.Length; i++)
			{
				int.TryParse(paramStr[i], out paramInt[i]);
			}

			switch (curCmd.pos)
			{
				//******************** 0 paramètres ****************************
				case LstPos.stop:
				case LstPos.hard_stop:
				case LstPos.reset:
				case LstPos.ask_position:
				case LstPos.ask_blocking:
				case LstPos.traj_finished:
				case LstPos.blocking_reset:
				case LstPos.ask_all_adc:
				case LstPos.magnet_front_pulse:
				case LstPos.magnet_back_pulse:
					m.sendCmd(curCmd.pos);
					break;
				//******************** 1 paramètres ****************************
				case LstPos.go_straight:
				case LstPos.turn_to:
				case LstPos.gen_func:
				//******************** 2 paramètres ****************************
				case LstPos.acceleration:
				case LstPos.speed:
				case LstPos.set_blocking:
				//******************** 3 paramètres ****************************
				case LstPos.goto_type:
				case LstPos.position_set:
				case LstPos.windows:
				//******************** 4 paramètres ****************************
				case LstPos.prepare_start:
					m.sendCmd(curCmd.pos, paramInt);
					break;
				//******************** 1 paramètres byte ***********************
				case LstPos.power:
				//******************** 2 paramètres byte ***********************
				case LstPos.pump:
				case LstPos.arm_mode:
					m.sendCmdByte(curCmd.pos, paramInt);
					break;
				//******************** Spécial ****************************
				case LstPos.drop:
					break;
				case LstPos.arm_calibration:
					break;
				default:
					break;
			}
		}

		private void sendCmdPump(int numPump, int val)
		{
			if (val != 0)
			{
				m.sendCmdByte(LstPos.pump, new int[] { numPump, 0 });
				System.Threading.Thread.Sleep(1000);
			}

			m.sendCmdByte(LstPos.pump, new int[] { numPump, val });
		}

		private void sendArmLeft(int typePos, int x, int y, int z)
		{
			m.sendCmd(LstPos.arm_position, new int[] { typePos | (0 << 8), x, y, z });
		}

		private void sendArmRight(int typePos, int x, int y, int z)
		{
			m.sendCmd(LstPos.arm_position, new int[] { typePos | (1 << 8), x, y, z });
		}

	

	}
}