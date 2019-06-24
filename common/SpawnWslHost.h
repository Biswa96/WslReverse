#ifndef SPAWNWSLHOST_H
#define SPAWNWSLHOST_H

BOOL
WINAPI
SpawnWslHost(HANDLE ServerHandle,
             GUID* InitiatedDistroID,
             GUID* LxInstanceID);

#endif /* SPAWNWSLHOST_H */
