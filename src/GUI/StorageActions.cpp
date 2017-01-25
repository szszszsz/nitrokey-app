
#include "StorageActions.h"
#include "src/ui/pindialog.h"
#include "src/utils/bool_values.h"
#include "libnitrokey/include/NitrokeyManager.h"

#ifdef Q_OS_LINUX
#include "systemutils.h"
#include <sys/mount.h> // for unmounting on linux
#endif                 // Q_OS_LINUX

#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
#include <unistd.h> //for sync syscall
#endif              // Q_OS_LINUX || Q_OS_MAC
#include <OwnSleep.h>
#include <src/ui/stick20lockfirmwaredialog.h>
#include <src/ui/stick20hiddenvolumedialog.h>

#define LOCAL_PASSWORD_SIZE 40


void unmountEncryptedVolumes() {
// TODO check will this work also on Mac
#if defined(Q_OS_LINUX)
  std::string endev = systemutils::getEncryptedDevice();
  if (endev.size() < 1)
    return;
  std::string mntdir = systemutils::getMntPoint(endev);
//  if (DebugingActive == TRUE)
  qDebug() << "Unmounting " << mntdir.c_str();
  // TODO polling with MNT_EXPIRE? test which will suit better
  // int err = umount2("/dev/nitrospace", MNT_DETACH);
  int err = umount(mntdir.c_str());
  if (err != 0) {
//    if (DebugingActive == TRUE)
    qDebug() << "Unmount error: " << strerror(errno);
  }
#endif // Q_OS_LINUX
}

void local_sync() {
  // TODO TEST unmount during/after big data transfer
  fflush(NULL); // for windows, not necessarly needed or working
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
  sync();
#endif // Q_OS_LINUX || Q_OS_MAC
  // manual says sync waits until it's done, but they
  // are not guaranteeing will this save data integrity anyway,
  // additional sleep should help
  OwnSleep::sleep(2);
  // unmount does sync on its own additionally (if successful)
  unmountEncryptedVolumes();
}


void StorageActions::startStick20EnableCryptedVolume() {
  bool ret;
  bool answer;

  if (TRUE == HiddenVolumeActive) {
    answer = csApplet()->yesOrNoBox(tr("This activity locks your hidden volume. Do you want to "
                                           "proceed?\nTo avoid data loss, please unmount the partitions before "
                                           "proceeding."), false);
    if (!answer)
      return;
  }

  PinDialog dialog(PinDialog::USER_PIN, nullptr);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    local_sync();
    const auto s = dialog.getPassword();
    auto m = nitrokey::NitrokeyManager::instance();
    m->unlock_encrypted_volume(s.data());
    CryptedVolumeActive = true;
    emit storageStatusChanged();
  }
}

void StorageActions::startStick20DisableCryptedVolume() {
  if (TRUE == CryptedVolumeActive) {
    bool answer = csApplet()->yesOrNoBox(tr("This activity locks your encrypted volume. Do you want to "
                                                "proceed?\nTo avoid data loss, please unmount the partitions before "
                                                "proceeding."), false);
    if (false == answer)
      return;

    local_sync();
    auto m = nitrokey::NitrokeyManager::instance();
    m->lock_device();
    CryptedVolumeActive = false;
    emit storageStatusChanged();
  }
}

void StorageActions::startStick20EnableHiddenVolume() {
  bool ret;
  bool answer;

  if (FALSE == CryptedVolumeActive) {
    csApplet()->warningBox(tr("Please enable the encrypted volume first."));
    return;
  }

  answer =
      csApplet()->yesOrNoBox(tr("This activity locks your encrypted volume. Do you want to "
                                    "proceed?\nTo avoid data loss, please unmount the partitions before "
                                    "proceeding."), true);
  if (!answer)
    return;

  PinDialog dialog(PinDialog::HIDDEN_VOLUME, nullptr);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    local_sync();
    // password[0] = 'P';
    auto s = dialog.getPassword();

    auto m = nitrokey::NitrokeyManager::instance();
    m->unlock_hidden_volume(s.data());
    HiddenVolumeActive = true;
    emit storageStatusChanged();
  }
}

void StorageActions::startStick20DisableHiddenVolume() {
  bool answer =
      csApplet()->yesOrNoBox(tr("This activity locks your hidden volume. Do you want to proceed?\nTo "
                                    "avoid data loss, please unmount the partitions before proceeding."), true);
  if (!answer)
    return;

  local_sync();
//  stick20SendCommand(STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI, password);
  auto m = nitrokey::NitrokeyManager::instance();
  m->lock_device(); //FIXME disable volume instead of locking the device
  HiddenVolumeActive = false;
  emit storageStatusChanged();
}

void StorageActions::startLockDeviceAction() {
  bool answer;

  if ((TRUE == CryptedVolumeActive) || (TRUE == HiddenVolumeActive)) {
    answer = csApplet()->yesOrNoBox(tr("This activity locks your encrypted volume. Do you want to "
                                           "proceed?\nTo avoid data loss, please unmount the partitions before "
                                           "proceeding."), true);
    if (!answer) {
      return;
    }
    local_sync();
  }

  auto m = nitrokey::NitrokeyManager::instance();
  m->lock_device();
  HiddenVolumeActive = false;
  CryptedVolumeActive = false;
  emit storageStatusChanged();

}

#include "stick20updatedialog.h"

void StorageActions::startStick20EnableFirmwareUpdate() {
  bool ret;

  UpdateDialog dialogUpdate(nullptr);

  ret = dialogUpdate.exec();
  if (QDialog::Accepted != ret) {
    return;
  }

  PinDialog dialog(PinDialog::FIRMWARE_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    // FIXME unmount all volumes and sync
    //TODO get password
//    dialog.getPassword((char *)password);

    //TODO add firmware update logic
//    stick20SendCommand(STICK20_CMD_ENABLE_FIRMWARE_UPDATE, password);
//    auto m = nitrokey::NitrokeyManager::instance();
//    m->enable_firmware_update();
  }
}


void StorageActions::startStick20ExportFirmwareToFile() {
  bool ret;

  PinDialog dialog(PinDialog::ADMIN_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    auto s = dialog.getPassword();

    auto m = nitrokey::NitrokeyManager::instance();
    m->export_firmware(s.data());
    //TODO UI add confirmation
  }
}

void StorageActions::startStick20DestroyCryptedVolume(int fillSDWithRandomChars) {
  int ret;
  bool answer;

  answer = csApplet()->yesOrNoBox(tr("WARNING: Generating new AES keys will destroy the encrypted volumes, "
                                         "hidden volumes, and password safe! Continue?"), false);
  if (answer) {
    PinDialog dialog(PinDialog::ADMIN_PIN);

    ret = dialog.exec();

    if (QDialog::Accepted == ret) {
      auto s = dialog.getPassword();

      auto m = nitrokey::NitrokeyManager::instance();
      m->build_aes_key(s.data());
      if (fillSDWithRandomChars != 0) {
        m->fill_SD_card_with_random_data(s.data());
      }
//      refreshStick20StatusData();
    }
  }
}

void StorageActions::startStick20FillSDCardWithRandomChars() {
  bool ret;
  PinDialog dialog(PinDialog::ADMIN_PIN);

  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    auto s = dialog.getPassword();
    auto m = nitrokey::NitrokeyManager::instance();
    m->fill_SD_card_with_random_data(s.data());
    emit storageStatusChanged();
  }
}

void StorageActions::startStick20ClearNewSdCardFound() {
  uint8_t password[LOCAL_PASSWORD_SIZE];
  bool ret;
  PinDialog dialog(PinDialog::ADMIN_PIN);

  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    auto s = dialog.getPassword();
    auto m = nitrokey::NitrokeyManager::instance();
    m->clear_new_sd_card_warning(s.data());
  }
}


void StorageActions::startStick20SetReadOnlyUncryptedVolume() {
  bool ret;

  PinDialog dialog(PinDialog::USER_PIN);

  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    const auto pass = dialog.getPassword();
    auto m = nitrokey::NitrokeyManager::instance();
    m->set_unencrypted_read_only(pass.data());
//    pass.safe_clear(); //TODO
    emit storageStatusChanged();
  }
}

void StorageActions::startStick20SetReadWriteUncryptedVolume() {
  uint8_t password[LOCAL_PASSWORD_SIZE];
  bool ret;

  PinDialog dialog(PinDialog::USER_PIN);

  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    const auto pass = dialog.getPassword();
    auto m = nitrokey::NitrokeyManager::instance();
    m->set_unencrypted_read_write(pass.data());
//    pass.safe_clear(); //TODO
    emit storageStatusChanged();
  }
}

void StorageActions::startStick20LockStickHardware() {
  uint8_t password[LOCAL_PASSWORD_SIZE];
  bool ret;
  stick20LockFirmwareDialog dialog(nullptr);

  ret = dialog.exec();
  if (QDialog::Accepted == ret) {
    PinDialog dialog(PinDialog::ADMIN_PIN);

    ret = dialog.exec();

    if (QDialog::Accepted == ret) {
      const auto pass = dialog.getPassword();
      auto m = nitrokey::NitrokeyManager::instance();
//    pass.safe_clear(); //TODO
// TODO     stick20SendCommand(STICK20_CMD_SEND_LOCK_STICK_HARDWARE, password);
    }
  }
}

void StorageActions::startStick20DebugAction() {
}

void StorageActions::startStick20SetupHiddenVolume() {
  bool ret;
  stick20HiddenVolumeDialog HVDialog(nullptr);

  if (FALSE == CryptedVolumeActive) {
    csApplet()->warningBox(tr("Please enable the encrypted volume first."));
    return;
  }

//FIXME this should be called from HVDialog
  HVDialog.SdCardHighWatermark_Read_Min = 0;
  HVDialog.SdCardHighWatermark_Read_Max = 100;
  HVDialog.SdCardHighWatermark_Write_Min = 0;
  HVDialog.SdCardHighWatermark_Write_Max = 100;
//  ret = cryptostick->getHighwaterMarkFromSdCard(
//      &HVDialog.SdCardHighWatermark_Write_Min, &HVDialog.SdCardHighWatermark_Write_Max,
//      &HVDialog.SdCardHighWatermark_Read_Min, &HVDialog.SdCardHighWatermark_Read_Max);
//  HVDialog.setHighWaterMarkText();
  ret = HVDialog.exec();

  if (true == ret) {
    const auto d = HVDialog.HV_Setup_st;
    auto p = std::string( reinterpret_cast< char const* >(d.HiddenVolumePassword_au8));
    auto m = nitrokey::NitrokeyManager::instance();
    m->create_hidden_volume(d.SlotNr_u8, d.StartBlockPercent_u8,
                            d.EndBlockPercent_u8, p.data());
  }
}

StorageActions::StorageActions(QObject *parent, Authentication *auth_admin, Authentication *auth_user) : QObject(
    parent), auth_admin(auth_admin), auth_user(auth_user) {

}