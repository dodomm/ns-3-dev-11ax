/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_PHY_H
#define WIFI_PHY_H

#include "ns3/event-id.h"
#include "ns3/deprecated.h"
#include "ns3/error-model.h"
#include "wifi-mpdu-type.h"
#include "wifi-phy-standard.h"
#include "interference-helper.h"
#include "wifi-phy-state-helper.h"
#include "wifi-ppdu.h"

namespace ns3 {

#define HE_PHY 125
#define VHT_PHY 126
#define HT_PHY 127

class Channel;
class NetDevice;
class MobilityModel;
class WifiPhyStateHelper;
class FrameCaptureModel;
class PreambleDetectionModel;
class ChannelBondingManager;
class WifiRadioEnergyModel;
class UniformRandomVariable;

typedef enum
{
  UNKNOWN = 0,
  UNSUPPORTED_SETTINGS,
  NOT_ALLOWED,
  ERRONEOUS_FRAME,
  MPDU_WITHOUT_PHY_HEADER,
  PREAMBLE_DETECT_FAILURE,
  L_SIG_FAILURE,
  SIG_A_FAILURE,
  PREAMBLE_DETECTION_PACKET_SWITCH,
  FRAME_CAPTURE_PACKET_SWITCH,
  OBSS_PD_CCA_RESET
} WifiPhyRxfailureReason;

/// SignalNoiseDbm structure
struct SignalNoiseDbm
{
  double signal; ///< in dBm
  double noise; ///< in dBm
};

/// MpduInfo structure
struct MpduInfo
{
  MpduType type; ///< type
  uint32_t mpduRefNumber; ///< MPDU ref number
};

// Parameters for receive HE preamble
struct HePreambleParameters
{
  double rssiW; ///< RSSI in W
  uint8_t bssColor; ///< BSS color
};

/// RxSignalInfo structure containing info on the received signal
struct RxSignalInfo
{
  double snr;  ///< SNR in linear scale
  double rssi; ///< RSSI in dBm
};

/**
 * \brief 802.11 PHY layer model
 * \ingroup wifi
 *
 */
class WifiPhy : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  WifiPhy ();
  virtual ~WifiPhy ();

  /**
   * A pair of a ChannelNumber and WifiPhyStandard
   */
  typedef std::pair<uint8_t, WifiPhyStandard> ChannelNumberStandardPair;
  
  /**
   * A pair of a center Frequency (MHz) and a ChannelWidth (MHz)
   */
  typedef std::pair<uint16_t, uint16_t> FrequencyWidthPair;
  
  /**
   * Return the WifiPhyStateHelper of this PHY
   *
   * \return the WifiPhyStateHelper of this PHY
   */
  Ptr<WifiPhyStateHelper> GetState (void) const;

  /**
   * \param callback the callback to invoke
   *        upon successful packet reception.
   */
  void SetReceiveOkCallback (RxOkCallback callback);
  /**
   * \param callback the callback to invoke
   *        upon erroneous packet reception.
   */
  void SetReceiveErrorCallback (RxErrorCallback callback);

  /**
   * \param listener the new listener
   *
   * Add the input listener to the list of objects to be notified of
   * PHY-level events.
   */
  void RegisterListener (WifiPhyListener *listener);
  /**
   * \param listener the listener to be unregistered
   *
   * Remove the input listener from the list of objects to be notified of
   * PHY-level events.
   */
  void UnregisterListener (WifiPhyListener *listener);

  /**
   * \param callback the callback to invoke when PHY capabilities have changed.
   */
  void SetCapabilitiesChangedCallback (Callback<void> callback);

  /**
   * Start receiving the PHY preamble of a PPDU (i.e. the first bit of the preamble has arrived).
   *
   * \param ppdu the arriving PPDU
   * \param rxPowersW the receive power in W per band
   */
  void StartReceivePreamble (Ptr<WifiPpdu> ppdu, RxPowerWattPerChannelBand rxPowersW);

  /**
   * Start receiving the PHY header of a PPDU (i.e. after the end of receiving the preamble).
   *
   * \param event the event holding incoming PPDU's information
   */
  void StartReceiveHeader (Ptr<Event> event);

  /**
   * Continue receiving the PHY header of a PPDU (i.e. after the end of receiving the legacy header part).
   *
   * \param event the event holding incoming PPDU's information
   */
  void ContinueReceiveHeader (Ptr<Event> event);

  /**
   * Start receiving the PSDU (i.e. the first symbol of the PSDU has arrived).
   *
   * \param event the event holding incoming PPDU's information
   */
  void StartReceivePayload (Ptr<Event> event);

  /**
   * Start receiving the PSDU (i.e. the first symbol of the PSDU has arrived) of an UL-OFDMA transmission.
   * This function is called upon the RX event corresponding to the OFDMA part of the UL MU PPDU.
   *
   * \param ppdu the arriving PPDU
   * \param rxPowersW the receive power in W per band
   */
  void StartReceiveOfdmaPayload (Ptr<WifiPpdu> ppdu, RxPowerWattPerChannelBand rxPowersW);

  /**
   * The last symbol of the PPDU has arrived.
   *
   * \param event the event holding incoming PPDU's information
   */
  void EndReceive (Ptr<Event> event);

  /**
   * For HE receptions only, check and possibly modify the transmit power restriction state at
   * the end of PPDU reception.
   */
  void EndReceiveInterBss (void);

  /**
   * \param psdus the PSDUs to send
   * \param txVector the TXVECTOR that has tx parameters such as mode, the transmission mode to use to send
   *        this PSDU, and txPowerLevel, a power level to use to send the whole PPDU. The real transmission
   *        power is calculated as txPowerMin + txPowerLevel * (txPowerMax - txPowerMin) / nTxLevels
   */
  void Send (WifiPsduMap psdus, WifiTxVector txVector);

  /**
   * \param ppdu the PPDU to send
   * \param txPowerLevel the power level to use
   *
   * Note that now that the content of the TXVECTOR is stored in the WifiPpdu through PHY headers,
   * the calling method has to specify the TX power level to use upon transmission.
   * Indeed the TXVECTOR obtained from WifiPpdu does not have this information set.
   */
  virtual void StartTx (Ptr<WifiPpdu> ppdu, uint8_t txPowerLevel) = 0;

  /**
   * Put in sleep mode.
   */
  void SetSleepMode (void);
  /**
   * Resume from sleep mode.
   */
  void ResumeFromSleep (void);
  /**
   * Put in off mode.
   */
  void SetOffMode (void);
  /**
   * Resume from off mode.
   */
  void ResumeFromOff (void);

  /**
   * \return true of the current state of the PHY layer is WifiPhy::IDLE, false otherwise.
   */
  bool IsStateIdle (void);
  /**
   * \return true of the current state of the PHY layer is WifiPhy::CCA_BUSY, false otherwise.
   */
  bool IsStateCcaBusy (void);
  /**
   * \return true of the current state of the PHY layer is WifiPhy::RX, false otherwise.
   */
  bool IsStateRx (void);
  /**
   * \return true of the current state of the PHY layer is WifiPhy::TX, false otherwise.
   */
  bool IsStateTx (void);
  /**
   * \return true of the current state of the PHY layer is WifiPhy::SWITCHING, false otherwise.
   */
  bool IsStateSwitching (void);
  /**
   * \return true if the current state of the PHY layer is WifiPhy::SLEEP, false otherwise.
   */
  bool IsStateSleep (void);
  /**
   * \return true if the current state of the PHY layer is WifiPhy::OFF, false otherwise.
   */
  bool IsStateOff (void);

  /**
   * \return the PHY state.
   * In case channel bonding is used, this returns the state of the primary channel.
   */
  WifiPhyState GetPhyState (void);

  /**
   * \param channelWidth the channel width to check
   * \param ccaThreshold the CCA threshold to consider
   * \return true if all the 20 MHz channels for the given channel width are idle, false otherwise
   */
  bool IsStateIdle (uint16_t channelWidth, double ccaThreshold);

  /**
   * todo
   */
  Time GetDelayUntilCcaEnd (double ccaThreshold, WifiSpectrumBand band);

  /**
   * \return the predicted delay until this PHY can become WifiPhy::IDLE.
   *
   * The PHY will never become WifiPhy::IDLE _before_ the delay returned by
   * this method but it could become really idle later.
   */
  Time GetDelayUntilIdle (void);

  /**
   * \param channelWidth the channel width to determine the number of 20 MHz bands to check
   * \param threshold the threshold used to decide whether the channel is WifiPhy::CCA_BUSY or WifiPhy::IDLE
   *
   * \return the minimum delay among the bonded channels since they are in WifiPhy::IDLE.
   */
  Time GetDelaySinceChannelIsIdle (uint16_t channelWidth, double threshold);

  /**
   * Return the start time of the last received packet.
   *
   * \return the start time of the last received packet
   */
  Time GetLastRxStartTime (void) const;

  /**
   * \param ppduDuration the duration of the HE TB PPDU
   * \param frequency the channel center frequency (MHz)
   *
   * \return the L-SIG length value corresponding to that HE TB PPDU duration.
   */
  static uint16_t ConvertHeTbPpduDurationToLSigLength (Time ppduDuration, uint16_t frequency);

  /**
   * \param length the L-SIG length value
   * \param txVector the TXVECTOR used for the transmission of this HE TB PPDU
   * \param frequency the channel center frequency (MHz)
   *
   * \return the duration of the HE TB PPDU corresponding to that L-SIG length value.
   */
  static Time ConvertLSigLengthToHeTbPpduDuration (uint16_t length, WifiTxVector txVector, uint16_t frequency);

  /**
   * \param size the number of bytes in the packet to send
   * \param txVector the TXVECTOR used for the transmission of this packet
   * \param frequency the channel center frequency (MHz)
   * \param staId the STA-ID of the recipient (only used for MU)
   *
   * \return the total amount of time this PHY will stay busy for the transmission of these bytes.
   */
  static Time CalculateTxDuration (uint32_t size, WifiTxVector txVector, uint16_t frequency, uint16_t staId = SU_STA_ID);
  /**
   * \param psduMap the PSDU(s) to transmit indexed by STA-ID
   * \param txVector the TXVECTOR used for the transmission of the PPDU
   * \param frequency the channel center frequency (MHz)
   *
   * \return the total amount of time this PHY will stay busy for the transmission of the PPDU
   */
  static Time CalculateTxDuration (WifiPsduMap psduMap, WifiTxVector txVector, uint16_t frequency);

  /**
   * \param txVector the transmission parameters used for this packet
   *
   * \return the total amount of time this PHY will stay busy for the transmission of the PLCP preamble and PLCP header.
   */
  static Time CalculatePlcpPreambleAndHeaderDuration (WifiTxVector txVector);
  /**
   *
   * \return the preamble detection duration, which is the time correletion needs to detect the start of an incoming frame.
   */
  static Time GetPreambleDetectionDuration (void);
  /**
   * \param txVector the transmission parameters used for this packet
   *
   * \return the training symbol duration
   */
  static Time GetPlcpTrainingSymbolDuration (WifiTxVector txVector);
  /**
   * \return the WifiMode used for the transmission of the HT-SIG and the HT training fields
   *         in Mixed Format and greenfield format PLCP header
   */
  static WifiMode GetHtPlcpHeaderMode ();
  /**
   * \return the WifiMode used for the transmission of the VHT-STF, VHT-LTF and VHT-SIG-B fields
   */
  static WifiMode GetVhtPlcpHeaderMode ();
  /**
   * \return the WifiMode used for the transmission of the HE-STF, HE-LTF and HE-SIG-B fields
   */
  static WifiMode GetHePlcpHeaderMode ();
  /*
   * Get the mode used for HE-SIG-B transmission.
   * This is applicable only for HE MU.
   * Get smallest HE MCS index among station's allocations and use the
   * VHT version of the index. This enables to have 800 ns GI, 52 data
   * tones, and 312.5 kHz spacing while ensuring that MCS will be decoded
   * by all stations.
   *
   * \param txVector the transmission parameters used for the HE MU PPDU
   *
   * \return the transmission mode used for HE-SIG-B
   */
  static WifiMode GetHeSigBMode (WifiTxVector txVector);
  /**
   * \param preamble the type of preamble
   *
   * \return the duration of the HT-SIG in Mixed Format and greenfield format PLCP header
   */
  static Time GetPlcpHtSigHeaderDuration (WifiPreamble preamble);
  /**
   * \param preamble the type of preamble
   *
   * \return the duration of the SIG-A1 in PLCP header
   */
  static Time GetPlcpSigA1Duration (WifiPreamble preamble);
  /**
   * \param preamble the type of preamble
   *
   * \return the duration of the SIG-A2 in PLCP header
   */
  static Time GetPlcpSigA2Duration (WifiPreamble preamble);
  /**
   * \param txVector the transmission parameters used for this packet
   *
   * \return the duration of the SIG-B in PLCP header
   */
  static Time GetPlcpSigBDuration (WifiTxVector txVector);
  /**
   * \param txVector the transmission parameters used for this packet
   *
   * \return the WifiMode used for the transmission of the PLCP header
   */
  static WifiMode GetPlcpHeaderMode (WifiTxVector txVector);
  /**
   * \param txVector the transmission parameters used for this packet
   *
   * \return the duration of the PLCP header
   */
  static Time GetPlcpHeaderDuration (WifiTxVector txVector);
  /**
   * \param txVector the transmission parameters used for this packet
   *
   * \return the duration of the PLCP preamble
   */
  static Time GetPlcpPreambleDuration (WifiTxVector txVector);
  /**
   * \param size the number of bytes in the packet to send
   * \param txVector the TXVECTOR used for the transmission of this packet
   * \param frequency the channel center frequency (MHz)
   * \param mpdutype the type of the MPDU as defined in WifiPhy::MpduType.
   * \param staId the STA-ID of the PSDU (only used for MU PPDUs)
   *
   * \return the duration of the PSDU
   */
  static Time GetPayloadDuration (uint32_t size, WifiTxVector txVector, uint16_t frequency, MpduType mpdutype = NORMAL_MPDU,
                                  uint16_t staId = SU_STA_ID);
  /**
   * \param size the number of bytes in the packet to send
   * \param txVector the TXVECTOR used for the transmission of this packet
   * \param frequency the channel center frequency (MHz)
   * \param mpdutype the type of the MPDU as defined in WifiPhy::MpduType.
   * \param incFlag this flag is used to indicate that the variables need to be update or not
   * This function is called a couple of times for the same packet so variables should not be increased each time.
   * \param totalAmpduSize the total size of the previously transmitted MPDUs for the concerned A-MPDU.
   * If incFlag is set, this parameter will be updated.
   * \param totalAmpduNumSymbols the number of symbols previously transmitted for the MPDUs in the concerned A-MPDU,
   * used for the computation of the number of symbols needed for the last MPDU.
   * If incFlag is set, this parameter will be updated.
   * \param staId the STA-ID of the PSDU (only used for MU PPDUs)
   *
   * \return the duration of the PSDU
   */
  static Time GetPayloadDuration (uint32_t size, WifiTxVector txVector, uint16_t frequency, MpduType mpdutype,
                                  bool incFlag, uint32_t &totalAmpduSize, double &totalAmpduNumSymbols,
                                  uint16_t staId);
  /**
   * \param txVector the transmission parameters used for this packet
   *
   * \return the duration until the start of the packet
   */
  static Time GetStartOfPacketDuration (WifiTxVector txVector);

  /**
   * The WifiPhy::GetNModes() and WifiPhy::GetMode() methods are used
   * (e.g., by a WifiRemoteStationManager) to determine the set of
   * transmission/reception modes that this WifiPhy(-derived class)
   * can support - a set of WifiMode objects which we call the
   * DeviceRateSet, and which is stored as WifiPhy::m_deviceRateSet.
   *
   * It is important to note that the DeviceRateSet is a superset (not
   * necessarily proper) of the OperationalRateSet (which is
   * logically, if not actually, a property of the associated
   * WifiRemoteStationManager), which itself is a superset (again, not
   * necessarily proper) of the BSSBasicRateSet.
   *
   * \return the number of transmission modes supported by this PHY.
   *
   * \sa WifiPhy::GetMode()
   */
  uint8_t GetNModes (void) const;
  /**
   * The WifiPhy::GetNModes() and WifiPhy::GetMode() methods are used
   * (e.g., by a WifiRemoteStationManager) to determine the set of
   * transmission/reception modes that this WifiPhy(-derived class)
   * can support - a set of WifiMode objects which we call the
   * DeviceRateSet, and which is stored as WifiPhy::m_deviceRateSet.
   *
   * It is important to note that the DeviceRateSet is a superset (not
   * necessarily proper) of the OperationalRateSet (which is
   * logically, if not actually, a property of the associated
   * WifiRemoteStationManager), which itself is a superset (again, not
   * necessarily proper) of the BSSBasicRateSet.
   *
   * \param mode index in array of supported modes
   *
   * \return the mode whose index is specified.
   *
   * \sa WifiPhy::GetNModes()
   */
  WifiMode GetMode (uint8_t mode) const;
  /**
   * Check if the given WifiMode is supported by the PHY.
   *
   * \param mode the wifi mode to check
   *
   * \return true if the given mode is supported,
   *         false otherwise
   */
  bool IsModeSupported (WifiMode mode) const;
  /**
   * Check if the given WifiMode is supported by the PHY.
   *
   * \param mcs the wifi mode to check
   *
   * \return true if the given mode is supported,
   *         false otherwise
   */
  bool IsMcsSupported (WifiMode mcs) const;
  /**
   * Check if the given MCS of the given modulation class is supported by the PHY.
   *
   * \param mc the modulation class
   * \param mcs the MCS value
   *
   * \return true if the given mode is supported,
   *         false otherwise
   */
  bool IsMcsSupported (WifiModulationClass mc, uint8_t mcs) const;

  /**
   * \param txVector the transmission vector
   * \param ber the probability of bit error rate
   *
   * \return the minimum snr which is required to achieve
   *          the requested ber for the specified transmission vector. (W/W)
   */
  double CalculateSnr (WifiTxVector txVector, double ber) const;

  /**
  * The WifiPhy::NBssMembershipSelectors() method is used
  * (e.g., by a WifiRemoteStationManager) to determine the set of
  * transmission/reception modes that this WifiPhy(-derived class)
  * can support - a set of WifiMode objects which we call the
  * BssMembershipSelectorSet, and which is stored as WifiPhy::m_bssMembershipSelectorSet.
  *
  * \return the membership selector whose index is specified.
  */
  uint8_t GetNBssMembershipSelectors (void) const;
  /**
  * The WifiPhy::BssMembershipSelector() method is used
  * (e.g., by a WifiRemoteStationManager) to determine the set of
  * transmission/reception modes that this WifiPhy(-derived class)
  * can support - a set of WifiMode objects which we call the
  * BssMembershipSelectorSet, and which is stored as WifiPhy::m_bssMembershipSelectorSet.
  *
  * \param selector index in array of supported memberships
  *
  * \return the memebership selector whose index is specified.
  */
  uint8_t GetBssMembershipSelector (uint8_t selector) const;
  /**
   * The WifiPhy::GetNMcs() method is used
   * (e.g., by a WifiRemoteStationManager) to determine the set of
   * transmission/reception MCS indexes that this WifiPhy(-derived class)
   * can support - a set of MCS indexes which we call the
   * DeviceMcsSet, and which is stored as WifiPhy::m_deviceMcsSet.
   *
   * \return the MCS index whose index is specified.
   */
  uint8_t GetNMcs (void) const;
  /**
   * The WifiPhy::GetMcs() method is used
   * (e.g., by a WifiRemoteStationManager) to determine the set of
   * transmission/reception MCS indexes that this WifiPhy(-derived class)
   * can support - a set of MCS indexes which we call the
   * DeviceMcsSet, and which is stored as WifiPhy::m_deviceMcsSet.
   *
   * \param mcs index in array of supported MCS
   *
   * \return the MCS index whose index is specified.
   */
  WifiMode GetMcs (uint8_t mcs) const;
  /**
   * Get the WifiMode object corresponding to the given MCS of the given
   * modulation class.
   *
   * \param modulation the modulation class
   * \param mcs the MCS value
   *
   * \return the WifiMode object corresponding to the given MCS of the given
   *         modulation class
   */
  WifiMode GetMcs (WifiModulationClass modulation, uint8_t mcs) const;
  /**
   * Get the WifiMode object corresponding to the given MCS of the
   * HT modulation class.
   *
   * \param mcs the MCS value
   *
   * \return the WifiMode object corresponding to the given MCS of the
   *         HT modulation class
   */
  static WifiMode GetHtMcs (uint8_t mcs);
  /**
   * Get the WifiMode object corresponding to the given MCS of the
   * VHT modulation class.
   *
   * \param mcs the MCS value
   *
   * \return the WifiMode object corresponding to the given MCS of the
   *         VHT modulation class
   */
  static WifiMode GetVhtMcs (uint8_t mcs);
  /**
   * Get the WifiMode object corresponding to the given MCS of the
   * HE modulation class.
   *
   * \param mcs the MCS value
   *
   * \return the WifiMode object corresponding to the given MCS of the
   *         HE modulation class
   */
  static WifiMode GetHeMcs (uint8_t mcs);

  /**
   * \brief Set channel number.
   *
   * Channel center frequency = Channel starting frequency + 5 MHz * (nch - 1)
   *
   * where Starting channel frequency is standard-dependent,
   * as defined in (Section 18.3.8.4.2 "Channel numbering"; IEEE Std 802.11-2012).
   * This method may fail to take action if the Phy model determines that
   * the channel number cannot be switched for some reason (e.g. sleep state)
   *
   * \param id the channel number
   */
  virtual void SetChannelNumber (uint8_t id);
  /**
   * Return current channel number.
   *
   * \return the current channel number
   */
  uint8_t GetChannelNumber (void) const;

  /**
   * \brief Set the primary 20 MHz channel number.
   *
   * \param id the primary channel number
   */
  void SetPrimaryChannelNumber (uint8_t id);
  /**
   * Return the primary channel number.
   *
   * \return the primary channel number
   */
  uint8_t GetPrimaryChannelNumber (void) const;

  /**
   * \return the required time for channel switch operation of this WifiPhy
   */
  Time GetChannelSwitchDelay (void) const;

  /**
   * Configure the PHY-level parameters for different Wi-Fi standard.
   *
   * \param standard the Wi-Fi standard
   */
  virtual void ConfigureStandard (WifiPhyStandard standard);

  /**
   * Get the configured Wi-Fi standard
   *
   * \return the Wi-Fi standard that has been configured
   */
  WifiPhyStandard GetStandard (void) const;

  /**
   * Add a channel definition to the WifiPhy.  The pair (channelNumber,
   * WifiPhyStandard) may then be used to lookup a pair (frequency,
   * channelWidth).
   *
   * If the channel is not already defined for the standard, the method
   * should return true; otherwise false.
   *
   * \param channelNumber the channel number to define
   * \param standard the applicable WifiPhyStandard
   * \param frequency the frequency (MHz)
   * \param channelWidth the channel width (MHz)
   *
   * \return true if the channel definition succeeded
   */
  bool DefineChannelNumber (uint8_t channelNumber, WifiPhyStandard standard, uint16_t frequency, uint16_t channelWidth);

  /**
   * Return the Channel this WifiPhy is connected to.
   *
   * \return the Channel this WifiPhy is connected to
   */
  virtual Ptr<Channel> GetChannel (void) const = 0;

  /**
   * Return a WifiMode for DSSS at 1Mbps.
   *
   * \return a WifiMode for DSSS at 1Mbps
   */
  static WifiMode GetDsssRate1Mbps ();
  /**
   * Return a WifiMode for DSSS at 2Mbps.
   *
   * \return a WifiMode for DSSS at 2Mbps
   */
  static WifiMode GetDsssRate2Mbps ();
  /**
   * Return a WifiMode for DSSS at 5.5Mbps.
   *
   * \return a WifiMode for DSSS at 5.5Mbps
   */
  static WifiMode GetDsssRate5_5Mbps ();
  /**
   * Return a WifiMode for DSSS at 11Mbps.
   *
   * \return a WifiMode for DSSS at 11Mbps
   */
  static WifiMode GetDsssRate11Mbps ();
  /**
   * Return a WifiMode for ERP-OFDM at 6Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 6Mbps
   */
  static WifiMode GetErpOfdmRate6Mbps ();
  /**
   * Return a WifiMode for ERP-OFDM at 9Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 9Mbps
   */
  static WifiMode GetErpOfdmRate9Mbps ();
  /**
   * Return a WifiMode for ERP-OFDM at 12Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 12Mbps
   */
  static WifiMode GetErpOfdmRate12Mbps ();
  /**
   * Return a WifiMode for ERP-OFDM at 18Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 18Mbps
   */
  static WifiMode GetErpOfdmRate18Mbps ();
  /**
   * Return a WifiMode for ERP-OFDM at 24Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 24Mbps
   */
  static WifiMode GetErpOfdmRate24Mbps ();
  /**
   * Return a WifiMode for ERP-OFDM at 36Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 36Mbps
   */
  static WifiMode GetErpOfdmRate36Mbps ();
  /**
   * Return a WifiMode for ERP-OFDM at 48Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 48Mbps
   */
  static WifiMode GetErpOfdmRate48Mbps ();
  /**
   * Return a WifiMode for ERP-OFDM at 54Mbps.
   *
   * \return a WifiMode for ERP-OFDM at 54Mbps
   */
  static WifiMode GetErpOfdmRate54Mbps ();
  /**
   * Return a WifiMode for OFDM at 6Mbps.
   *
   * \return a WifiMode for OFDM at 6Mbps
   */
  static WifiMode GetOfdmRate6Mbps ();
  /**
   * Return a WifiMode for OFDM at 9Mbps.
   *
   * \return a WifiMode for OFDM at 9Mbps
   */
  static WifiMode GetOfdmRate9Mbps ();
  /**
   * Return a WifiMode for OFDM at 12Mbps.
   *
   * \return a WifiMode for OFDM at 12Mbps
   */
  static WifiMode GetOfdmRate12Mbps ();
  /**
   * Return a WifiMode for OFDM at 18Mbps.
   *
   * \return a WifiMode for OFDM at 18Mbps
   */
  static WifiMode GetOfdmRate18Mbps ();
  /**
   * Return a WifiMode for OFDM at 24Mbps.
   *
   * \return a WifiMode for OFDM at 24Mbps
   */
  static WifiMode GetOfdmRate24Mbps ();
  /**
   * Return a WifiMode for OFDM at 36Mbps.
   *
   * \return a WifiMode for OFDM at 36Mbps
   */
  static WifiMode GetOfdmRate36Mbps ();
  /**
   * Return a WifiMode for OFDM at 48Mbps.
   *
   * \return a WifiMode for OFDM at 48Mbps
   */
  static WifiMode GetOfdmRate48Mbps ();
  /**
   * Return a WifiMode for OFDM at 54Mbps.
   *
   * \return a WifiMode for OFDM at 54Mbps
   */
  static WifiMode GetOfdmRate54Mbps ();
  /**
   * Return a WifiMode for OFDM at 3Mbps with 10MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 3Mbps with 10MHz channel spacing
   */
  static WifiMode GetOfdmRate3MbpsBW10MHz ();
  /**
   * Return a WifiMode for OFDM at 4.5Mbps with 10MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 4.5Mbps with 10MHz channel spacing
   */
  static WifiMode GetOfdmRate4_5MbpsBW10MHz ();
  /**
   * Return a WifiMode for OFDM at 6Mbps with 10MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 6Mbps with 10MHz channel spacing
   */
  static WifiMode GetOfdmRate6MbpsBW10MHz ();
  /**
   * Return a WifiMode for OFDM at 9Mbps with 10MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 9Mbps with 10MHz channel spacing
   */
  static WifiMode GetOfdmRate9MbpsBW10MHz ();
  /**
   * Return a WifiMode for OFDM at 12Mbps with 10MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 12Mbps with 10MHz channel spacing
   */
  static WifiMode GetOfdmRate12MbpsBW10MHz ();
  /**
   * Return a WifiMode for OFDM at 18Mbps with 10MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 18Mbps with 10MHz channel spacing
   */
  static WifiMode GetOfdmRate18MbpsBW10MHz ();
  /**
   * Return a WifiMode for OFDM at 24Mbps with 10MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 24Mbps with 10MHz channel spacing
   */
  static WifiMode GetOfdmRate24MbpsBW10MHz ();
  /**
   * Return a WifiMode for OFDM at 27Mbps with 10MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 27Mbps with 10MHz channel spacing
   */
  static WifiMode GetOfdmRate27MbpsBW10MHz ();
  /**
   * Return a WifiMode for OFDM at 1.5Mbps with 5MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 1.5Mbps with 5MHz channel spacing
   */
  static WifiMode GetOfdmRate1_5MbpsBW5MHz ();
  /**
   * Return a WifiMode for OFDM at 2.25Mbps with 5MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 2.25Mbps with 5MHz channel spacing
   */
  static WifiMode GetOfdmRate2_25MbpsBW5MHz ();
  /**
   * Return a WifiMode for OFDM at 3Mbps with 5MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 3Mbps with 5MHz channel spacing
   */
  static WifiMode GetOfdmRate3MbpsBW5MHz ();
  /**
   * Return a WifiMode for OFDM at 4.5Mbps with 5MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 4.5Mbps with 5MHz channel spacing
   */
  static WifiMode GetOfdmRate4_5MbpsBW5MHz ();
  /**
   * Return a WifiMode for OFDM at 6Mbps with 5MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 6Mbps with 5MHz channel spacing
   */
  static WifiMode GetOfdmRate6MbpsBW5MHz ();
  /**
   * Return a WifiMode for OFDM at 9Mbps with 5MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 9Mbps with 5MHz channel spacing
   */
  static WifiMode GetOfdmRate9MbpsBW5MHz ();
  /**
   * Return a WifiMode for OFDM at 12Mbps with 5MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 12Mbps with 5MHz channel spacing
   */
  static WifiMode GetOfdmRate12MbpsBW5MHz ();
  /**
   * Return a WifiMode for OFDM at 13.5Mbps with 5MHz channel spacing.
   *
   * \return a WifiMode for OFDM at 13.5Mbps with 5MHz channel spacing
   */
  static WifiMode GetOfdmRate13_5MbpsBW5MHz ();

  /**
   * Return MCS 0 from HT MCS values.
   *
   * \return MCS 0 from HT MCS values
   */
  static WifiMode GetHtMcs0 ();
  /**
   * Return MCS 1 from HT MCS values.
   *
   * \return MCS 1 from HT MCS values
   */
  static WifiMode GetHtMcs1 ();
  /**
   * Return MCS 2 from HT MCS values.
   *
   * \return MCS 2 from HT MCS values
   */
  static WifiMode GetHtMcs2 ();
  /**
   * Return MCS 3 from HT MCS values.
   *
   * \return MCS 3 from HT MCS values
   */
  static WifiMode GetHtMcs3 ();
  /**
   * Return MCS 4 from HT MCS values.
   *
   * \return MCS 4 from HT MCS values
   */
  static WifiMode GetHtMcs4 ();
  /**
   * Return MCS 5 from HT MCS values.
   *
   * \return MCS 5 from HT MCS values
   */
  static WifiMode GetHtMcs5 ();
  /**
   * Return MCS 6 from HT MCS values.
   *
   * \return MCS 6 from HT MCS values
   */
  static WifiMode GetHtMcs6 ();
  /**
   * Return MCS 7 from HT MCS values.
   *
   * \return MCS 7 from HT MCS values
   */
  static WifiMode GetHtMcs7 ();
  /**
   * Return MCS 8 from HT MCS values.
   *
   * \return MCS 8 from HT MCS values
   */
  static WifiMode GetHtMcs8 ();
  /**
   * Return MCS 9 from HT MCS values.
   *
   * \return MCS 9 from HT MCS values
   */
  static WifiMode GetHtMcs9 ();
  /**
   * Return MCS 10 from HT MCS values.
   *
   * \return MCS 10 from HT MCS values
   */
  static WifiMode GetHtMcs10 ();
  /**
   * Return MCS 11 from HT MCS values.
   *
   * \return MCS 11 from HT MCS values
   */
  static WifiMode GetHtMcs11 ();
  /**
   * Return MCS 12 from HT MCS values.
   *
   * \return MCS 12 from HT MCS values
   */
  static WifiMode GetHtMcs12 ();
  /**
   * Return MCS 13 from HT MCS values.
   *
   * \return MCS 13 from HT MCS values
   */
  static WifiMode GetHtMcs13 ();
  /**
   * Return MCS 14 from HT MCS values.
   *
   * \return MCS 14 from HT MCS values
   */
  static WifiMode GetHtMcs14 ();
  /**
   * Return MCS 15 from HT MCS values.
   *
   * \return MCS 15 from HT MCS values
   */
  static WifiMode GetHtMcs15 ();
  /**
   * Return MCS 16 from HT MCS values.
   *
   * \return MCS 16 from HT MCS values
   */
  static WifiMode GetHtMcs16 ();
  /**
   * Return MCS 17 from HT MCS values.
   *
   * \return MCS 17 from HT MCS values
   */
  static WifiMode GetHtMcs17 ();
  /**
   * Return MCS 18 from HT MCS values.
   *
   * \return MCS 18 from HT MCS values
   */
  static WifiMode GetHtMcs18 ();
  /**
   * Return MCS 19 from HT MCS values.
   *
   * \return MCS 19 from HT MCS values
   */
  static WifiMode GetHtMcs19 ();
  /**
   * Return MCS 20 from HT MCS values.
   *
   * \return MCS 20 from HT MCS values
   */
  static WifiMode GetHtMcs20 ();
  /**
   * Return MCS 21 from HT MCS values.
   *
   * \return MCS 21 from HT MCS values
   */
  static WifiMode GetHtMcs21 ();
  /**
   * Return MCS 22 from HT MCS values.
   *
   * \return MCS 22 from HT MCS values
   */
  static WifiMode GetHtMcs22 ();
  /**
   * Return MCS 23 from HT MCS values.
   *
   * \return MCS 23 from HT MCS values
   */
  static WifiMode GetHtMcs23 ();
  /**
   * Return MCS 24 from HT MCS values.
   *
   * \return MCS 24 from HT MCS values
   */
  static WifiMode GetHtMcs24 ();
  /**
   * Return MCS 25 from HT MCS values.
   *
   * \return MCS 25 from HT MCS values
   */
  static WifiMode GetHtMcs25 ();
  /**
   * Return MCS 26 from HT MCS values.
   *
   * \return MCS 26 from HT MCS values
   */
  static WifiMode GetHtMcs26 ();
  /**
   * Return MCS 27 from HT MCS values.
   *
   * \return MCS 27 from HT MCS values
   */
  static WifiMode GetHtMcs27 ();
  /**
   * Return MCS 28 from HT MCS values.
   *
   * \return MCS 28 from HT MCS values
   */
  static WifiMode GetHtMcs28 ();
  /**
   * Return MCS 29 from HT MCS values.
   *
   * \return MCS 29 from HT MCS values
   */
  static WifiMode GetHtMcs29 ();
  /**
   * Return MCS 30 from HT MCS values.
   *
   * \return MCS 30 from HT MCS values
   */
  static WifiMode GetHtMcs30 ();
  /**
   * Return MCS 31 from HT MCS values.
   *
   * \return MCS 31 from HT MCS values
   */
  static WifiMode GetHtMcs31 ();

  /**
   * Return MCS 0 from VHT MCS values.
   *
   * \return MCS 0 from VHT MCS values
   */
  static WifiMode GetVhtMcs0 ();
  /**
   * Return MCS 1 from VHT MCS values.
   *
   * \return MCS 1 from VHT MCS values
   */
  static WifiMode GetVhtMcs1 ();
  /**
   * Return MCS 2 from VHT MCS values.
   *
   * \return MCS 2 from VHT MCS values
   */
  static WifiMode GetVhtMcs2 ();
  /**
   * Return MCS 3 from VHT MCS values.
   *
   * \return MCS 3 from VHT MCS values
   */
  static WifiMode GetVhtMcs3 ();
  /**
   * Return MCS 4 from VHT MCS values.
   *
   * \return MCS 4 from VHT MCS values
   */
  static WifiMode GetVhtMcs4 ();
  /**
   * Return MCS 5 from VHT MCS values.
   *
   * \return MCS 5 from VHT MCS values
   */
  static WifiMode GetVhtMcs5 ();
  /**
   * Return MCS 6 from VHT MCS values.
   *
   * \return MCS 6 from VHT MCS values
   */
  static WifiMode GetVhtMcs6 ();
  /**
   * Return MCS 7 from VHT MCS values.
   *
   * \return MCS 7 from VHT MCS values
   */
  static WifiMode GetVhtMcs7 ();
  /**
   * Return MCS 8 from VHT MCS values.
   *
   * \return MCS 8 from VHT MCS values
   */
  static WifiMode GetVhtMcs8 ();
  /**
   * Return MCS 9 from VHT MCS values.
   *
   * \return MCS 9 from VHT MCS values
   */
  static WifiMode GetVhtMcs9 ();

  /**
   * Return MCS 0 from HE MCS values.
   *
   * \return MCS 0 from HE MCS values
   */
  static WifiMode GetHeMcs0 ();
  /**
   * Return MCS 1 from HE MCS values.
   *
   * \return MCS 1 from HE MCS values
   */
  static WifiMode GetHeMcs1 ();
  /**
   * Return MCS 2 from HE MCS values.
   *
   * \return MCS 2 from HE MCS values
   */
  static WifiMode GetHeMcs2 ();
  /**
   * Return MCS 3 from HE MCS values.
   *
   * \return MCS 3 from HE MCS values
   */
  static WifiMode GetHeMcs3 ();
  /**
   * Return MCS 4 from HE MCS values.
   *
   * \return MCS 4 from HE MCS values
   */
  static WifiMode GetHeMcs4 ();
  /**
   * Return MCS 5 from HE MCS values.
   *
   * \return MCS 5 from HE MCS values
   */
  static WifiMode GetHeMcs5 ();
  /**
   * Return MCS 6 from HE MCS values.
   *
   * \return MCS 6 from HE MCS values
   */
  static WifiMode GetHeMcs6 ();
  /**
   * Return MCS 7 from HE MCS values.
   *
   * \return MCS 7 from HE MCS values
   */
  static WifiMode GetHeMcs7 ();
  /**
   * Return MCS 8 from HE MCS values.
   *
   * \return MCS 8 from HE MCS values
   */
  static WifiMode GetHeMcs8 ();
  /**
   * Return MCS 9 from HE MCS values.
   *
   * \return MCS 9 from HE MCS values
   */
  static WifiMode GetHeMcs9 ();
  /**
   * Return MCS 10 from HE MCS values.
   *
   * \return MCS 10 from HE MCS values
   */
  static WifiMode GetHeMcs10 ();
  /**
   * Return MCS 11 from HE MCS values.
   *
   * \return MCS 11 from HE MCS values
   */
  static WifiMode GetHeMcs11 ();

  /**
   * Public method used to fire a PhyTxBegin trace.
   * Implemented for encapsulation purposes.
   *
   * \param psdus the PSDUs being transmitted (only one unless MU transmission)
   * \param txPowerW the transmit power in Watts
   */
  void NotifyTxBegin (WifiPsduMap psdus, double txPowerW);
  /**
   * Public method used to fire a PhyTxEnd trace.
   * Implemented for encapsulation purposes.
   *
   * \param psdu the PSDU being transmitted
   */
  void NotifyTxEnd (Ptr<const WifiPsdu> psdu);
  /**
   * Public method used to fire a PhyTxDrop trace.
   * Implemented for encapsulation purposes.
   *
   * \param psdu the PSDU being transmitted
   */
  void NotifyTxDrop (Ptr<const WifiPsdu> psdu);
  /**
   * Public method used to fire a PhyRxBegin trace.
   * Implemented for encapsulation purposes.
   *
   * \param psdu the PSDU being transmitted
   * \param rxPowersW the receive power per channel band in Watts
   */
  void NotifyRxBegin (Ptr<const WifiPsdu> psdu, RxPowerWattPerChannelBand rxPowersW);
  /**
   * Public method used to fire a PhyRxEnd trace.
   * Implemented for encapsulation purposes.
   *
   * \param psdu the PSDU being transmitted
   */
  void NotifyRxEnd (Ptr<const WifiPsdu> psdu);
  /**
   * Public method used to fire a PhyRxDrop trace.
   * Implemented for encapsulation purposes.
   *
   * \param psdu the PSDU being transmitted
   * \param reason the reason the packet was dropped
   */
  void NotifyRxDrop (Ptr<const WifiPsdu> psdu, WifiPhyRxfailureReason reason);

  /**
   * Public method used to fire a MonitorSniffer trace for a wifi PSDU being received.
   * Implemented for encapsulation purposes.
   * This method will extract all MPDUs if packet is an A-MPDU and will fire tracedCallback.
   * The A-MPDU reference number (RX side) is set within the method. It must be a different value
   * for each A-MPDU but the same for each subframe within one A-MPDU.
   *
   * \param psdu the PSDU being received
   * \param channelFreqMhz the frequency in MHz at which the packet is
   *        received. Note that in real devices this is normally the
   *        frequency to which  the receiver is tuned, and this can be
   *        different than the frequency at which the packet was originally
   *        transmitted. This is because it is possible to have the receiver
   *        tuned on a given channel and still to be able to receive packets
   *        on a nearby channel.
   * \param txVector the TXVECTOR that holds rx parameters
   * \param signalNoise signal power and noise power in dBm (noise power includes the noise figure)
   * \param statusPerMpdu reception status per MPDU
   */
  void NotifyMonitorSniffRx (Ptr<const WifiPsdu> psdu,
                             uint16_t channelFreqMhz,
                             WifiTxVector txVector,
                             SignalNoiseDbm signalNoise,
                             std::vector<bool> statusPerMpdu);

  /**
   * TracedCallback signature for monitor mode receive events.
   *
   *
   * \param packet the packet being received
   * \param channelFreqMhz the frequency in MHz at which the packet is
   *        received. Note that in real devices this is normally the
   *        frequency to which  the receiver is tuned, and this can be
   *        different than the frequency at which the packet was originally
   *        transmitted. This is because it is possible to have the receiver
   *        tuned on a given channel and still to be able to receive packets
   *        on a nearby channel.
   * \param txVector the TXVECTOR that holds rx parameters
   * \param aMpdu the type of the packet (0 is not A-MPDU, 1 is a MPDU that is part of an A-MPDU and 2 is the last MPDU in an A-MPDU)
   *        and the A-MPDU reference number (must be a different value for each A-MPDU but the same for each subframe within one A-MPDU)
   * \param signalNoise signal power and noise power in dBm
   * \todo WifiTxVector should be passed by const reference because
   * of its size.
   */
  typedef void (* MonitorSnifferRxCallback)(Ptr<const Packet> packet,
                                            uint16_t channelFreqMhz,
                                            WifiTxVector txVector,
                                            MpduInfo aMpdu,
                                            SignalNoiseDbm signalNoise);

  /**
   * Public method used to fire a MonitorSniffer trace for a wifi PSDU being transmitted.
   * Implemented for encapsulation purposes.
   * This method will extract all MPDUs if packet is an A-MPDU and will fire tracedCallback.
   * The A-MPDU reference number (RX side) is set within the method. It must be a different value
   * for each A-MPDU but the same for each subframe within one A-MPDU.
   *
   * \param psdu the PSDU being received
   * \param channelFreqMhz the frequency in MHz at which the packet is
   *        transmitted.
   * \param txVector the TXVECTOR that holds tx parameters
   */
  void NotifyMonitorSniffTx (Ptr<const WifiPsdu> psdu,
                             uint16_t channelFreqMhz,
                             WifiTxVector txVector);

  /**
   * TracedCallback signature for monitor mode transmit events.
   *
   * \param packet the packet being transmitted
   * \param channelFreqMhz the frequency in MHz at which the packet is
   *        transmitted.
   * \param txVector the TXVECTOR that holds tx parameters
   * \param aMpdu the type of the packet (0 is not A-MPDU, 1 is a MPDU that is part of an A-MPDU and 2 is the last MPDU in an A-MPDU)
   *        and the A-MPDU reference number (must be a different value for each A-MPDU but the same for each subframe within one A-MPDU)
   * \todo WifiTxVector should be passed by const reference because
   * of its size.
   */
  typedef void (* MonitorSnifferTxCallback)(const Ptr<const Packet> packet,
                                            uint16_t channelFreqMhz,
                                            WifiTxVector txVector,
                                            MpduInfo aMpdu);

  /**
   * Public method used to fire a EndOfHePreamble trace once both HE SIG fields have been received, as well as training fields.
   *
   * \param params the HE preamble parameters
   */
  void NotifyEndOfHePreamble (HePreambleParameters params);

  /**
   * TracedCallback signature for end of HE-SIG-A events.
   *
   *
   * \param params the HE preamble parameters
   */
  typedef void (* EndOfHePreambleCallback)(HePreambleParameters params);

  /**
   * TracedCallback signature for start of PSDU reception events.
   *
   * \param txVector the TXVECTOR decoded from the PHY header
   * \param psduDuration the duration of the PSDU
   */
  typedef void (* PhyRxPayloadBeginTracedCallback)(WifiTxVector txVector, Time psduDuration);

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model. Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  virtual int64_t AssignStreams (int64_t stream);

  /**
   * Sets the energy detection threshold (dBm).
   * The energy of a received signal should be higher than
   * this threshold (dbm) to allow the PHY layer to detect the signal.
   *
   * \param threshold the energy detection threshold in dBm
   *
   * \deprecated
   */
  void SetEdThreshold (double threshold);
  /**
   * Sets the receive sensitivity threshold (dBm).
   * The energy of a received signal should be higher than
   * this threshold to allow the PHY layer to detect the signal.
   *
   * \param threshold the receive sensitivity threshold in dBm
   */
  void SetRxSensitivity (double threshold);
  /**
   * Return the receive sensitivity threshold (dBm).
   *
   * \return the receive sensitivity threshold in dBm
   */
  double GetRxSensitivity (void) const;
  /**
   * Sets the CCA threshold (dBm). The energy of a received signal
   * should be higher than this threshold to allow the PHY
   * layer to declare CCA BUSY state.
   *
   * \param threshold the CCA threshold in dBm
   */
  void SetCcaEdThreshold (double threshold);
  /**
   * Return the CCA threshold (dBm).
   *
   * \return the CCA threshold in dBm
   */
  double GetCcaEdThreshold (void) const;
  /**
   * Add a CCA threshold (dBm) for the secondary channels. The energy of a received signal
   * should be higher than this threshold to allow the PHY layer to declare CCA BUSY state.
   *
   * \param threshold the CCA threshold in dBm to be added for the secondary channels
   */
  void AddCcaEdThresholdSecondary (double threshold);
  /**
   * Return the default CCA threshold (dBm) for the secondary channels.
   *
   * \return the default CCA threshold (dBm) for the secondary channels
   */
  double GetDefaultCcaEdThresholdSecondary (void) const;
  /**
   * Sets the RX loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver.
   *
   * \param noiseFigureDb noise figure in dB
   */
  void SetRxNoiseFigure (double noiseFigureDb);
  /**
   * Sets the minimum available transmission power level (dBm).
   *
   * \param start the minimum transmission power level (dBm)
   */
  void SetTxPowerStart (double start);
  /**
   * Return the minimum available transmission power level (dBm).
   *
   * \return the minimum available transmission power level (dBm)
   */
  double GetTxPowerStart (void) const;
  /**
   * Sets the maximum available transmission power level (dBm).
   *
   * \param end the maximum transmission power level (dBm)
   */
  void SetTxPowerEnd (double end);
  /**
   * Return the maximum available transmission power level (dBm).
   *
   * \return the maximum available transmission power level (dBm)
   */
  double GetTxPowerEnd (void) const;
  /**
   * Sets the number of transmission power levels available between the
   * minimum level and the maximum level. Transmission power levels are
   * equally separated (in dBm) with the minimum and the maximum included.
   *
   * \param n the number of available levels
   */
  void SetNTxPower (uint8_t n);
  /**
   * Return the number of available transmission power levels.
   *
   * \return the number of available transmission power levels
   */
  uint8_t GetNTxPower (void) const;
  /**
   * Sets the transmission gain (dB).
   *
   * \param gain the transmission gain in dB
   */
  void SetTxGain (double gain);
  /**
   * Return the transmission gain (dB).
   *
   * \return the transmission gain in dB
   */
  double GetTxGain (void) const;
  /**
   * Sets the reception gain (dB).
   *
   * \param gain the reception gain in dB
   */
  void SetRxGain (double gain);
  /**
   * Return the reception gain (dB).
   *
   * \return the reception gain in dB
   */
  double GetRxGain (void) const;

  /**
   * Sets the device this PHY is associated with.
   *
   * \param device the device this PHY is associated with
   */
  void SetDevice (const Ptr<NetDevice> device);
  /**
   * Return the device this PHY is associated with
   *
   * \return the device this PHY is associated with
   */
  Ptr<NetDevice> GetDevice (void) const;
  /**
   * \brief assign a mobility model to this device
   *
   * This method allows a user to specify a mobility model that should be
   * associated with this physical layer.  Calling this method is optional
   * and only necessary if the user wants to override the mobility model
   * that is aggregated to the node.
   *
   * \param mobility the mobility model this PHY is associated with
   */
  void SetMobility (const Ptr<MobilityModel> mobility);
  /**
   * Return the mobility model this PHY is associated with.
   * This method will return either the mobility model that has been
   * explicitly set by a call to YansWifiPhy::SetMobility(), or else
   * will return the mobility model (if any) that has been aggregated
   * to the node.
   *
   * \return the mobility model this PHY is associated with
   */
  Ptr<MobilityModel> GetMobility (void) const;

  /**
   * \param freq the operating center frequency (MHz) on this node.
   */
  virtual void SetFrequency (uint16_t freq);
  /**
   * \return the operating center frequency (MHz)
   */
  uint16_t GetFrequency (void) const;
  /**
   * \param antennas the number of antennas on this node.
   */
  void SetNumberOfAntennas (uint8_t antennas);
  /**
   * \return the number of antennas on this device
   */
  uint8_t GetNumberOfAntennas (void) const;
  /**
   * \param streams the maximum number of supported TX spatial streams.
   */
  void SetMaxSupportedTxSpatialStreams (uint8_t streams);
  /**
   * \return the maximum number of supported TX spatial streams
   */
  uint8_t GetMaxSupportedTxSpatialStreams (void) const;
  /**
   * \param streams the maximum number of supported RX spatial streams.
   */
  void SetMaxSupportedRxSpatialStreams (uint8_t streams);
  /**
   * \return the maximum number of supported RX spatial streams
   */
  uint8_t GetMaxSupportedRxSpatialStreams (void) const;
  /**
   * Enable or disable support for HT/VHT short guard interval.
   *
   * \param shortGuardInterval Enable or disable support for short guard interval
   *
   * \deprecated
   */
  void SetShortGuardInterval (bool shortGuardInterval);
  /**
   * Return whether short guard interval is supported.
   *
   * \return true if short guard interval is supported, false otherwise
   *
   * \deprecated
   */
  bool GetShortGuardInterval (void) const;
  /**
   * \param guardInterval the supported HE guard interval
   *
   * \deprecated
   */
  void SetGuardInterval (Time guardInterval);
  /**
   * \return the supported HE guard interval
   *
   * \deprecated
   */
  Time GetGuardInterval (void) const;
  /**
   * Enable or disable Greenfield support.
   *
   * \param greenfield Enable or disable Greenfield
   *
   * \deprecated
   */
  void SetGreenfield (bool greenfield);
  /**
   * Return whether Greenfield is supported.
   *
   * \return true if Greenfield is supported, false otherwise
   *
   * \deprecated
   */
  bool GetGreenfield (void) const;
  /**
   * Enable or disable short PLCP preamble.
   *
   * \param preamble sets whether short PLCP preamble is supported or not
   */
  void SetShortPlcpPreambleSupported (bool preamble);
  /**
   * Return whether short PLCP preamble is supported.
   *
   * \returns true if short PLCP preamble is supported, false otherwise
   */
  bool GetShortPlcpPreambleSupported (void) const;

  /**
   * Sets the error rate model.
   *
   * \param rate the error rate model
   */
  void SetErrorRateModel (const Ptr<ErrorRateModel> rate);
  /**
   * Attach a receive ErrorModel to the WifiPhy.
   *
   * The WifiPhy may optionally include an ErrorModel in
   * the packet receive chain. The error model is additive
   * to any modulation-based error model based on SNR, and
   * is typically used to force specific packet losses or
   * for testing purposes.
   *
   * \param em Ptr to the ErrorModel.
   */
  void SetPostReceptionErrorModel (const Ptr<ErrorModel> em);
  /**
   * Sets the frame capture model.
   *
   * \param frameCaptureModel the frame capture model
   */
  void SetFrameCaptureModel (const Ptr<FrameCaptureModel> frameCaptureModel);
  /**
   * Sets the preamble detection model.
   *
   * \param preambleDetectionModel the preamble detection model
   */
  void SetPreambleDetectionModel (const Ptr<PreambleDetectionModel> preambleDetectionModel);
  /**
   * Sets the channel bonding manager.
   *
   * \param channelBondingManager the channel bonding manager
   */
  void SetChannelBondingManager (const Ptr<ChannelBondingManager> channelBondingManager);
  /**
   * Sets the wifi radio energy model.
   *
   * \param wifiRadioEnergyModel the wifi radio energy model
   */
  void SetWifiRadioEnergyModel (const Ptr<WifiRadioEnergyModel> wifiRadioEnergyModel);
  /**
   * Set PCF Interframe Space (PIFS) of this PHY.
   *
   * \param pifs PIFS of this PHY
   */
  void SetPifs (Time pifs);
  /**
   * Return PCF Interframe Space (PIFS) of this PHY.
   *
   * \return PIFS
   */
  Time GetPifs (void) const;
  /**
   * \return the channel width
   */
  uint16_t GetChannelWidth (void) const;
  /**
   * \param mode the WifiMode that is selected for the transmission
   *
   * \return the usable channel width for the transmission
   */
  uint16_t GetUsableChannelWidth (WifiMode mode);
  /**
   * \param channelwidth channel width
   */
  virtual void SetChannelWidth (uint16_t channelwidth);
  /**
   * \param channelwidth channel width (in MHz) to support
   */
  void AddSupportedChannelWidth (uint16_t channelwidth);
  /**
   * \return a vector containing the supported channel widths, values in MHz
   */
  std::vector<uint16_t> GetSupportedChannelWidthSet (void) const;

  /**
   * Get the power of the given power level in dBm.
   * In SpectrumWifiPhy implementation, the power levels are equally spaced (in dBm).
   *
   * \param power the power level
   *
   * \return the transmission power in dBm at the given power level
   */
  double GetPowerDbm (uint8_t power) const;

  /**
   * Reset PHY to IDLE, with some potential TX power restrictions for the next transmission.
   *
   * \param powerRestricted flag whether the transmit power is restricted for the next transmission
   * \param txPowerMaxSiso the SISO transmit power retriction for the next transmission
   * \param txPowerMaxMimo the MIMO transmit power retriction for the next transmission
   */
  void ResetCca (bool powerRestricted, double txPowerMaxSiso = 0, double txPowerMaxMimo = 0);
  /**
   * Compute the transmit power (in dBm) for the next transmission.
   *
   * \param txVector the TXVECTOR
   * \return the transmit power in dBm for the next transmission
   */
  double GetTxPowerForTransmission (WifiTxVector txVector) const;
  /**
   * Notify the PHY that an access to the channel was requested.
   * This is typically called by the channel access manager to
   * to notify the PHY about an ongoing transmission.
   * The PHY will use this information to determine whether
   * it should use power restriction as imposed by OBSS_PD SR.
   */
  void NotifyChannelAccessRequested (void);


protected:
  // Inherited
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

  /**
   * Check if PHY state should move to CCA busy state based on current
   * state of interference tracker.  In this model, CCA becomes busy when
   * the aggregation of all signals as tracked by the InterferenceHelper
   * class is higher than the CcaEdThreshold
   */
  void MaybeCcaBusy (void);

  /*
   * Reset data upon end of TX or RX
   */
  void Reset (void);

  /**
   * Get the center frequency of the channel corresponding the current TxVector rather than
   * that of the supported channel width.
   * Consider that this "primary channel" is on the lower part for the time being.
   *
   * \param currentWidth the channel width (in MHz)
   * \return the center frequency corresponding to the channel width to be used
   */
  uint16_t GetCenterFrequencyForChannelWidth (uint16_t currentWidth) const;

  /**
   * The default implementation does nothing and returns true.  This method
   * is typically called internally by SetChannelNumber ().
   *
   * \brief Perform any actions necessary when user changes channel number
   * \param id channel number to try to switch to
   * \return true if WifiPhy can actually change the number; false if not
   * \see SetChannelNumber
   */
  bool DoChannelSwitch (uint8_t id);
  /**
   * The default implementation does nothing and returns true.  This method
   * is typically called internally by SetFrequency ().
   *
   * \brief Perform any actions necessary when user changes frequency
   * \param frequency frequency to try to switch to
   * \return true if WifiPhy can actually change the frequency; false if not
   * \see SetFrequency
   */
  bool DoFrequencySwitch (uint16_t frequency);

  /**
   * Return the STA ID that has been assigned to the station this PHY belongs to.
   * This is typically called for MU PPDUs, in order to pick the correct PSDU.
   *
   * \param ppdu the PPDU for which the STA ID is requested
   * \return the STA ID
   */
  virtual uint16_t GetStaId (const Ptr<const WifiPpdu> ppdu) const;

  //TODO
  uint8_t GetPrimaryBandIndex (uint16_t currentWidth) const;

  /**
   * Get the start band index and the stop band index for a given band
   *
   * \param bandWidth the width of the band to be returned (MHz)
   * \param bandIndex the index of the band to be returned
   *
   * \return a pair of start and stop indexes that defines the band
   */
  virtual WifiSpectrumBand GetBand (uint16_t bandWidth, uint8_t bandIndex = 0);

  /**
   * \param channelWidth the total channel width (MHz) used for the OFDMA transmission
   * \param range the subcarrier range of the HE RU
   * \return the converted subcarriers
   *
   * This is a helper function to convert HE RU subcarriers, which are relative to the center frequency subcarrier, to the indexes used by the Spectrum model.
   */
  virtual WifiSpectrumBand ConvertHeRuSubcarriers (uint16_t channelWidth, HeRu::SubcarrierRange range) const;

  /**
   * Get the RU band used to transmit a PSDU to a given STA in a HE MU PPDU
   *
   * \param txVector the TXVECTOR used for the transmission
   * \param staId the STA-ID of the recipient
   *
   * \return the RU band used to transmit a PSDU to a given STA in a HE MU PPDU
   */
  WifiSpectrumBand GetRuBand (WifiTxVector txVector, uint16_t staId);

  InterferenceHelper m_interference;   //!< Pointer to InterferenceHelper
  Ptr<UniformRandomVariable> m_random; //!< Provides uniform random variables.
  Ptr<WifiPhyStateHelper> m_state;     //!< Pointer to WifiPhyStateHelper

  uint32_t m_txMpduReferenceNumber;    //!< A-MPDU reference number to identify all transmitted subframes belonging to the same received A-MPDU
  uint32_t m_rxMpduReferenceNumber;    //!< A-MPDU reference number to identify all received subframes belonging to the same received A-MPDU

  EventId m_endPlcpRxEvent;            //!< the end of PLCP receive event

  std::vector <EventId> m_endOfMpduEvents; //!< the end of MPDU events (only used for A-MPDUs)

  std::vector <EventId> m_endRxEvents; //!< the end of receive events (only one unless UL MU reception)
  std::vector <EventId> m_endPreambleDetectionEvents; //!< the end of preamble detection events

  Ptr<Event> m_currentEvent; //!< Hold the current event
  std::map <uint64_t /* UID*/, Ptr<Event> > m_currentPreambleEvents; //!< store event associated to a PPDU (that has a unique ID) whose preamble is being received

  uint64_t m_currentHeTbPpduUid;   //!< UID of the HE TB PPDU being received
  uint64_t m_previouslyRxPpduUid;  //!< UID of the previously received PPDU (reused by HE TB PPDUs), reset to UINT64_MAX upon transmission

  static uint64_t m_globalPpduUid;     //!< Global counter of the PPDU UID


private:
  /**
   * \brief post-construction setting of frequency and/or channel number
   *
   * This method exists to handle the fact that two attribute values,
   * Frequency and ChannelNumber, are coupled.  The initialization of
   * these values needs to be deferred until after attribute construction
   * time, to avoid static initialization order issues.  This method is
   * typically called either when ConfigureStandard () is called or when
   * DoInitialize () is called.
   */
  void InitializeFrequencyChannelNumber (void);
  /**
   * Configure WifiPhy with appropriate channel frequency and
   * supported rates for 802.11a standard.
   */
  void Configure80211a (void);
  /**
   * Configure WifiPhy with appropriate channel frequency and
   * supported rates for 802.11b standard.
   */
  void Configure80211b (void);
  /**
   * Configure WifiPhy with appropriate channel frequency and
   * supported rates for 802.11g standard.
   */
  void Configure80211g (void);
  /**
   * Configure WifiPhy with appropriate channel frequency and
   * supported rates for 802.11a standard with 10MHz channel spacing.
   */
  void Configure80211_10Mhz (void);
  /**
   * Configure WifiPhy with appropriate channel frequency and
   * supported rates for 802.11a standard with 5MHz channel spacing.
   */
  void Configure80211_5Mhz ();
  /**
   * Configure WifiPhy with appropriate channel frequency and
   * supported rates for holland.
   */
  void ConfigureHolland (void);
  /**
   * Configure WifiPhy with appropriate channel frequency and
   * supported rates for 802.11n standard.
   */
  void Configure80211n (void);
  /**
   * Configure WifiPhy with appropriate channel frequency and
   * supported rates for 802.11ac standard.
   */
  void Configure80211ac (void);
  /**
   * Configure WifiPhy with appropriate channel frequency and
   * supported rates for 802.11ax standard.
   */
  void Configure80211ax (void);
  /**
   * Configure the device Mcs set with the appropriate HtMcs modes for
   * the number of available transmit spatial streams
   */
  void ConfigureHtDeviceMcsSet (void);
  /**
   * Add the given MCS to the device MCS set.
   *
   * \param mode the MCS to add to the device MCS set
   */
  void PushMcs (WifiMode mode);
  /**
   * Rebuild the mapping of MCS values to indices in the device MCS set.
   */
  void RebuildMcsMap (void);
  /**
   * Configure the PHY-level parameters for different Wi-Fi standard.
   * This method is called when defaults for each standard must be
   * selected.
   *
   * \param standard the Wi-Fi standard
   */
  void ConfigureDefaultsForStandard (WifiPhyStandard standard);
  /**
   * Configure the PHY-level parameters for different Wi-Fi standard.
   * This method is called when the Frequency or ChannelNumber attributes
   * are set by the user.  If the Frequency or ChannelNumber are valid for
   * the standard, they are used instead.
   *
   * \param standard the Wi-Fi standard
   */
  void ConfigureChannelForStandard (WifiPhyStandard standard);

  /**
   * Look for channel number matching the frequency and width
   * \param frequency The center frequency to use
   * \param width The channel width to use
   * \return the channel number if found, zero if not
   */
  uint8_t FindChannelNumberForFrequencyWidth (uint16_t frequency, uint16_t width) const;
  /**
   * Lookup frequency/width pair for channelNumber/standard pair
   * \param channelNumber The channel number to check
   * \param standard The WifiPhyStandard to check
   * \return the FrequencyWidthPair found
   */
  FrequencyWidthPair GetFrequencyWidthForChannelNumberStandard (uint8_t channelNumber, WifiPhyStandard standard) const;

  /**
   * Due to newly arrived signal, the current reception cannot be continued and has to be aborted
   * \param reason the reason the reception is aborted
   *
   */
  void AbortCurrentReception (WifiPhyRxfailureReason reason);

  /**
   * Starting receiving the PPDU after having detected the medium is idle or after a reception switch.
   *
   * \param event the event holding incoming PPDU's information
   */
  void StartRx (Ptr<Event> event);
  /**
   * Get the reception status for the provided MPDU and notify.
   *
   * \param psdu the arriving MPDU formatted as a PSDU
   * \param event the event holding incoming PPDU's information
   * \param staId the station ID of the PSDU (only used for MU)
   * \param relativeMpduStart the relative start time of the MPDU within the A-MPDU. 0 for normal MPDUs
   * \param mpduDuration the duration of the MPDU
   *
   * \return information on MPDU reception: status, signal power (dBm), and noise power (in dBm)
   */
  std::pair<bool, SignalNoiseDbm> GetReceptionStatus (Ptr<const WifiPsdu> psdu,
                                                      Ptr<Event> event, uint16_t staId,
                                                      Time relativeMpduStart,
                                                      Time mpduDuration);
  /**
   * The last symbol of an MPDU in an A-MPDU has arrived.
   *
   * \param event the event holding incoming PPDU's information
   * \param psdu the arriving MPDU formatted as a PSDU containing a normal MPDU
   * \param mpduIndex the index of the MPDU within the A-MPDU
   * \param relativeMpduStart the relative start time of the MPDU within the A-MPDU.
   * \param mpduDuration the duration of the MPDU
   */  
  void EndOfMpdu (Ptr<Event> event, Ptr<const WifiPsdu> psdu, size_t mpduIndex, Time relativeStart, Time mpduDuration);

  /**
   * Schedule end of MPDUs events.
   *
   * \param event the event holding incoming PPDU's information
   */
  void ScheduleEndOfMpdus (Ptr<Event> event);

  /**
   * Get the PSDU addressed to that PHY in a PPDU (useful for MU PPDU).
   *
   * \param ppdu the PPDU to extract the PSDU from
   * \return the PSDU addressed to that PHY
   */
  Ptr<const WifiPsdu> GetAddressedPsduInPpdu (Ptr<const WifiPpdu> ppdu) const;

  /**
   * The trace source fired when a packet begins the transmission process on
   * the medium.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet>, double > m_phyTxBeginTrace;

  /**
   * The trace source fired when a packet ends the transmission process on
   * the medium.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyTxEndTrace;

  /**
   * The trace source fired when the phy layer drops a packet as it tries
   * to transmit it.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyTxDropTrace;

  /**
   * The trace source fired when a packet begins the reception process from
   * the medium.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet>, RxPowerWattPerChannelBand > m_phyRxBeginTrace;

  /**
   * The trace source fired when the reception of the PHY payload (PSDU) begins.
   *
   * This traced callback models the behavior of the PHY-RXSTART
   * primitive which is launched upon correct decoding of
   * the PHY header and support of modes within.
   * We thus assume that it is sent just before starting
   * the decoding of the payload, since it's there that
   * support of the header's content is checked. In addition,
   * it's also at that point that the correct decoding of
   * HT-SIG, VHT-SIGs, and HE-SIGs are checked.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<WifiTxVector, Time> m_phyRxPayloadBeginTrace;

  /**
   * The trace source fired when a packet ends the reception process from
   * the medium.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyRxEndTrace;

  /**
   * The trace source fired when the phy layer drops a packet it has received.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet>, WifiPhyRxfailureReason > m_phyRxDropTrace;

  /**
   * A trace source that emulates a wifi device in monitor mode
   * sniffing a packet being received.
   *
   * As a reference with the real world, firing this trace
   * corresponds in the madwifi driver to calling the function
   * ieee80211_input_monitor()
   *
   * \see class CallBackTraceSource
   * \todo WifiTxVector and signalNoiseDbm should be be passed as
   * const  references because of their sizes.
   */
  TracedCallback<Ptr<const Packet>, uint16_t, WifiTxVector, MpduInfo, SignalNoiseDbm> m_phyMonitorSniffRxTrace;

  /**
   * A trace source that emulates a wifi device in monitor mode
   * sniffing a packet being transmitted.
   *
   * As a reference with the real world, firing this trace
   * corresponds in the madwifi driver to calling the function
   * ieee80211_input_monitor()
   *
   * \see class CallBackTraceSource
   * \todo WifiTxVector should be passed by const reference because
   * of its size.
   */
  TracedCallback<Ptr<const Packet>, uint16_t, WifiTxVector, MpduInfo> m_phyMonitorSniffTxTrace;

  /**
   * A trace source that indiates the end of both HE SIG fields as well as training fields for received 802.11ax packets
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<HePreambleParameters> m_phyEndOfHePreambleTrace;

  /**
   * This vector holds the set of transmission modes that this
   * WifiPhy(-derived class) can support. In conversation we call this
   * the DeviceRateSet (not a term you'll find in the standard), and
   * it is a superset of standard-defined parameters such as the
   * OperationalRateSet, and the BSSBasicRateSet (which, themselves,
   * have a superset/subset relationship).
   *
   * Mandatory rates relevant to this WifiPhy can be found by
   * iterating over this vector looking for WifiMode objects for which
   * WifiMode::IsMandatory() is true.
   *
   * A quick note is appropriate here (well, here is as good a place
   * as any I can find)...
   *
   * In the standard there is no text that explicitly precludes
   * production of a device that does not support some rates that are
   * mandatory (according to the standard) for PHYs that the device
   * happens to fully or partially support.
   *
   * This approach is taken by some devices which choose to only support,
   * for example, 6 and 9 Mbps ERP-OFDM rates for cost and power
   * consumption reasons (i.e., these devices don't need to be designed
   * for and waste current on the increased linearity requirement of
   * higher-order constellations when 6 and 9 Mbps more than meet their
   * data requirements). The wording of the standard allows such devices
   * to have an OperationalRateSet which includes 6 and 9 Mbps ERP-OFDM
   * rates, despite 12 and 24 Mbps being "mandatory" rates for the
   * ERP-OFDM PHY.
   *
   * Now this doesn't actually have any impact on code, yet. It is,
   * however, something that we need to keep in mind for the
   * future. Basically, the key point is that we can't be making
   * assumptions like "the Operational Rate Set will contain all the
   * mandatory rates".
   */
  WifiModeList m_deviceRateSet;
  WifiModeList m_deviceMcsSet; //!< the device MCS set
  /// Maps MCS values to indices in m_deviceMcsSet, for HT, VHT and HE modulation classes
  std::map<WifiModulationClass, std::map<uint8_t /* MCS value */, uint8_t /* index */>> m_mcsIndexMap;

  std::vector<uint8_t> m_bssMembershipSelectorSet; //!< the BSS membership selector set

  WifiPhyStandard m_standard;                       //!< WifiPhyStandard
  bool m_isConstructed;                             //!< true when ready to set frequency
  uint16_t m_channelCenterFrequency;                //!< Center frequency in MHz
  uint16_t m_initialFrequency;                      //!< Store frequency until initialization
  bool m_frequencyChannelNumberInitialized;         //!< Store initialization state
  uint16_t m_channelWidth;                          //!< Channel width

  double   m_rxSensitivityW;           //!< Receive sensitivity threshold in watts
  double   m_ccaEdThresholdW;          //!< Clear channel assessment (CCA) threshold for primary channel in watts

  std::vector<double> m_ccaEdThresholdsSecondaryW; //!< Clear channel assessment (CCA) thresholds for secondary channel(s) in watts

  double   m_txGainDb;       //!< Transmission gain (dB)
  double   m_rxGainDb;       //!< Reception gain (dB)
  double   m_txPowerBaseDbm; //!< Minimum transmission power (dBm)
  double   m_txPowerEndDbm;  //!< Maximum transmission power (dBm)
  uint8_t  m_nTxPower;       //!< Number of available transmission power levels

  bool m_powerRestricted;  //!< Flag whether transmit power is retricted by OBSS PD SR
  double m_txPowerMaxSiso; //!< SISO maximum transmit power due to OBSS PD SR power restriction
  double m_txPowerMaxMimo; //!< MIMO maximum transmit power due to OBSS PD SR power restriction
  bool m_channelAccessRequested;

  bool     m_greenfield;         //!< Flag if GreenField format is supported (deprecated)
  bool     m_shortGuardInterval; //!< Flag if HT/VHT short guard interval is supported (deprecated)
  bool     m_shortPreamble;      //!< Flag if short PLCP preamble is supported

  Time m_guardInterval; //!< Supported HE guard interval (deprecated)

  uint8_t m_numberOfAntennas;  //!< Number of transmitters
  uint8_t m_txSpatialStreams;  //!< Number of supported TX spatial streams
  uint8_t m_rxSpatialStreams;  //!< Number of supported RX spatial streams

  typedef std::map<ChannelNumberStandardPair,FrequencyWidthPair> ChannelToFrequencyWidthMap; //!< channel to frequency width map typedef
  static ChannelToFrequencyWidthMap m_channelToFrequencyWidth; //!< the channel to frequency width map

  std::vector<uint16_t> m_supportedChannelWidthSet; //!< Supported channel width
  uint8_t               m_channelNumber;            //!< Operating channel number
  uint8_t               m_primaryChannelNumber;     //!< Primary 20 MHz channel number
  uint8_t               m_initialChannelNumber;     //!< Initial channel number

  Time m_channelSwitchDelay;     //!< Time required to switch between channel

  Ptr<NetDevice>     m_device;   //!< Pointer to the device
  Ptr<MobilityModel> m_mobility; //!< Pointer to the mobility model

  Ptr<FrameCaptureModel> m_frameCaptureModel; //!< Frame capture model
  Ptr<PreambleDetectionModel> m_preambleDetectionModel; //!< Preamble detection model
  Ptr<ChannelBondingManager> m_channelBondingManager; //!< Channel bonding manager
  Ptr<WifiRadioEnergyModel> m_wifiRadioEnergyModel; //!< Wifi radio energy model
  Ptr<ErrorModel> m_postReceptionErrorModel; //!< Error model for receive packet events
  Time m_timeLastPreambleDetected; //!< Record the time the last preamble was detected
  Time m_pifs; //!< PCF Interframe Space (PIFS) duration

  /**
   * A pair of a UID and STA_ID
   */
  typedef std::pair <uint64_t /* uid */, uint16_t /* staId */> UidStaIdPair;

  std::map<UidStaIdPair, std::vector<bool> > m_statusPerMpduMap; //!< Map of the current reception status per MPDU that is filled in as long as MPDUs are being processed by the PHY in case of an A-MPDU
  std::map<UidStaIdPair, SignalNoiseDbm> m_signalNoiseMap; //!< Map of the latest signal power and noise power in dBm (noise power includes the noise figure)

  bool m_ofdmaStarted; //!< Flag whether the reception of the OFDMA part has started (only used for UL-OFDMA)

  Callback<void> m_capabilitiesChangedCallback; //!< Callback when PHY capabilities changed
};

/**
 * \param os          output stream
 * \param state       wifi state to stringify
 * \return output stream
 */
std::ostream& operator<< (std::ostream& os, WifiPhyState state);

/**
 * \param os           output stream
 * \param rxSignalInfo received signal info to stringify
 * \return output stream
 */
std::ostream& operator<< (std::ostream& os, RxSignalInfo rxSignalInfo);

} //namespace ns3

#endif /* WIFI_PHY_H */
