import math
import random as rand


def l2_pairwise_distance(v1, v2):
    """Euclidean distance from each point in v1 to each point in v2

    Args:
        v1: list of point 1
        v2: list of point 2

    Returns:
        distance matrix between each point in v1 and v2
    """
    nrow = len(v1)
    ncol = len(v2)
    dist_mat = [[0 for _ in range(ncol)] for __ in range(nrow)]
    for i in range(nrow):
        for j in range(ncol):
            dist_mat[i][j] = math.sqrt((v1[i] - v2[j]) ** 2)
    return dist_mat


def calculate_error(k_mean_matrix):
    """Calculate the sum of distance from each point to its nearest cluster center

    Args:
        k_mean_matrix: distance matrix of point to cluster center

    Returns:
        Sum of distance from each point to its nearest cluster center
    """
    return sum([min(dist) for dist in k_mean_matrix])


def k_mean(x_input, n_cluster=3, n_iter=100):
    """Perform 1-D k-mean clustering on a list of number x_input

    Args:
        x_input: list of number
        n_cluster: number of cluster
        n_iter: number of iteration

    Returns:
        centers: list of n_cluster elements containing the cluster centers
        min_dist_idx: list of len(x_input) elements containing the nearest cluster center's id
        error_value: sum of all distance from each point to its nearest cluster center
    """
    results = []
    for _ in range(10):
        error_value = 0
        rand.seed(None)
        centers = sorted([rand.uniform(0.0, 100.0) for i in range(n_cluster)])
        min_dist_idx = [0] * len(x_input)
        i = 0
        while i < n_iter:
            failed = False
            dist_mat = l2_pairwise_distance(x_input, centers)
            error_value = calculate_error(dist_mat)
            min_dist_idx = [dist.index(min(dist)) for dist in dist_mat]
            centers = [0] * n_cluster
            count = [0] * n_cluster
            for j in range(len(x_input)):
                centers[min_dist_idx[j]] += x_input[j]
                count[min_dist_idx[j]] += 1

            for j in range(n_cluster):
                if count[j] == 0:
                    centers = sorted([rand.uniform(0.0, 100.0) for i in range(n_cluster)])
                    failed = True
                    break

            if failed:
                i = 0
                continue

            for j in range(n_cluster):
                centers[j] = centers[j] / (count[j])
            i += 1

        results.append((centers, min_dist_idx, error_value))

    return min(results, key=lambda x: x[2])
