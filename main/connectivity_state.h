#ifndef _CONNECTIVITY_STATE_H_
#define _CONNECTIVITY_STATE_H_

enum ConnectivityState {
    kConnectivityUnknown,
    kConnectivityConnecting,
    kConnectivityNetworkAvailable,
    kConnectivityNetworkUnavailable,
    kConnectivityProvisioning,
};

#endif  // _CONNECTIVITY_STATE_H_
