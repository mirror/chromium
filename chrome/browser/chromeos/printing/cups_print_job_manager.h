// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PRINTING_CUPS_PRINT_JOB_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_PRINTING_CUPS_PRINT_JOB_MANAGER_H_

#include <memory>
#include <vector>

#include "base/observer_list.h"
#include "chrome/browser/chromeos/printing/cups_print_job.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace chromeos {

class CupsPrintJobNotificationManager;

class CupsPrintJobManager : public KeyedService {
 public:
  class Observer {
   public:
    // Note, |job| could be deleted depending on status.
    virtual void OnPrintJobUpdated(CupsPrintJob* job) {}
   protected:
    virtual ~Observer() {}
  };

  static CupsPrintJobManager* CreateInstance(Profile* profile);

  explicit CupsPrintJobManager(Profile* profile);
  ~CupsPrintJobManager() override;

  // KeyedService override:
  void Shutdown() override;

  // Cancel a print job |job|. Note the |job| will be deleted after cancelled.
  // There will be no notifications after cancellation.
  virtual void CancelPrintJob(CupsPrintJob* job) = 0;
  virtual bool SuspendPrintJob(CupsPrintJob* job) = 0;
  virtual bool ResumePrintJob(CupsPrintJob* job) = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  void NotifyJobUpdated(CupsPrintJob* job);

  Profile* profile_;
  std::unique_ptr<CupsPrintJobNotificationManager> notification_manager_;
  base::ObserverList<Observer> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CupsPrintJobManager);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PRINTING_CUPS_PRINT_JOB_MANAGER_H_
